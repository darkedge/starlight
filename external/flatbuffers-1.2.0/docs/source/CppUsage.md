# Use in C++

Assuming you have written a schema using the above language in say
`mygame.fbs` (FlatBuffer Schema, though the extension doesn't matter),
you've generated a C++ header called `mygame_generated.h` using the
compiler (e.g. `flatc -c mygame.fbs`), you can now start using this in
your program by including the header. As noted, this header relies on
`flatbuffers/flatbuffers.h`, which should be in your include path.

### Writing in C++

To start creating a buffer, create an instance of `FlatBufferBuilder`
which will contain the buffer as it grows:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    FlatBufferBuilder fbb;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Before we serialize a Monster, we need to first serialize any objects
that are contained there-in, i.e. we serialize the data tree using
depth first, pre-order traversal. This is generally easy to do on
any tree structures. For example:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    auto name = fbb.CreateString("MyMonster");

    unsigned char inv[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    auto inventory = fbb.CreateVector(inv, 10);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`CreateString` and `CreateVector` serialize these two built-in
datatypes, and return offsets into the serialized data indicating where
they are stored, such that `Monster` below can refer to them.

`CreateString` can also take an `std::string`, or a `const char *` with
an explicit length, and is suitable for holding UTF-8 and binary
data if needed.

`CreateVector` can also take an `std::vector`. The
offset it returns is typed, i.e. can only be used to set fields of the
correct type below. To create a vector of struct objects (which will
be stored as contiguous memory in the buffer, use `CreateVectorOfStructs`
instead.

To create a vector of nested objects (e.g. tables, strings or other vectors)
collect their offsets in a temporary array/vector, then call `CreateVector`
on that (see e.g. the array of strings example in `test.cpp`
`CreateFlatBufferTest`).

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    Vec3 vec(1, 2, 3);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`Vec3` is the first example of code from our generated
header. Structs (unlike tables) translate to simple structs in C++, so
we can construct them in a familiar way.

We have now serialized the non-scalar components of of the monster
example, so we could create the monster something like this:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    auto mloc = CreateMonster(fbb, &vec, 150, 80, name, inventory, Color_Red, 0, Any_NONE);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Note that we're passing `150` for the `mana` field, which happens to be the
default value: this means the field will not actually be written to the buffer,
since we'll get that value anyway when we query it. This is a nice space
savings, since it is very common for fields to be at their default. It means
we also don't need to be scared to add fields only used in a minority of cases,
since they won't bloat up the buffer sizes if they're not actually used.

We do something similarly for the union field `test` by specifying a `0` offset
and the `NONE` enum value (part of every union) to indicate we don't actually
want to write this field. You can use `0` also as a default for other
non-scalar types, such as strings, vectors and tables. To pass an actual
table, pass a preconstructed table as `mytable.Union()` that corresponds to
union enum you're passing.

Tables (like `Monster`) give you full flexibility on what fields you write
(unlike `Vec3`, which always has all fields set because it is a `struct`).
If you want even more control over this (i.e. skip fields even when they are
not default), instead of the convenient `CreateMonster` call we can also
build the object field-by-field manually:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    MonsterBuilder mb(fbb);
    mb.add_pos(&vec);
    mb.add_hp(80);
    mb.add_name(name);
    mb.add_inventory(inventory);
    auto mloc = mb.Finish();
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We start with a temporary helper class `MonsterBuilder` (which is
defined in our generated code also), then call the various `add_`
methods to set fields, and `Finish` to complete the object. This is
pretty much the same code as you find inside `CreateMonster`, except
we're leaving out a few fields. Fields may also be added in any order,
though orderings with fields of the same size adjacent
to each other most efficient in size, due to alignment. You should
not nest these Builder classes (serialize your
data in pre-order).

Regardless of whether you used `CreateMonster` or `MonsterBuilder`, you
now have an offset to the root of your data, and you can finish the
buffer using:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    FinishMonsterBuffer(fbb, mloc);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The buffer is now ready to be stored somewhere, sent over the network,
be compressed, or whatever you'd like to do with it. You can access the
start of the buffer with `fbb.GetBufferPointer()`, and it's size from
`fbb.GetSize()`.

Calling code may take ownership of the buffer with `fbb.ReleaseBufferPointer()`.
Should you do it, the `FlatBufferBuilder` will be in an invalid state,
and *must* be cleared before it can be used again.
However, it also means you are able to destroy the builder while keeping
the buffer in your application.

`samples/sample_binary.cpp` is a complete code sample similar to
the code above, that also includes the reading code below.

### Reading in C++

If you've received a buffer from somewhere (disk, network, etc.) you can
directly start traversing it using:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    auto monster = GetMonster(buffer_pointer);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`monster` is of type `Monster *`, and points to somewhere *inside* your
buffer (root object pointers are not the same as `buffer_pointer` !).
If you look in your generated header, you'll see it has
convenient accessors for all fields, e.g.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    assert(monster->hp() == 80);
    assert(monster->mana() == 150);  // default
    assert(strcmp(monster->name()->c_str(), "MyMonster") == 0);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

These should all be true. Note that we never stored a `mana` value, so
it will return the default.

To access sub-objects, in this case the `Vec3`:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    auto pos = monster->pos();
    assert(pos);
    assert(pos->z() == 3);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If we had not set the `pos` field during serialization, it would be
`NULL`.

Similarly, we can access elements of the inventory array:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    auto inv = monster->inventory();
    assert(inv);
    assert(inv->Get(9) == 9);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

### Mutating FlatBuffers

As you saw above, typically once you have created a FlatBuffer, it is
read-only from that moment on. There are however cases where you have just
received a FlatBuffer, and you'd like to modify something about it before
sending it on to another recipient. With the above functionality, you'd have
to generate an entirely new FlatBuffer, while tracking what you modify in your
own data structures. This is inconvenient.

For this reason FlatBuffers can also be mutated in-place. While this is great
for making small fixes to an existing buffer, you generally want to create
buffers from scratch whenever possible, since it is much more efficient and
the API is much more general purpose.

To get non-const accessors, invoke `flatc` with `--gen-mutable`.

Similar to the reading API above, you now can:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    auto monster = GetMutableMonster(buffer_pointer);  // non-const
    monster->mutate_hp(10);                      // Set table field.
    monster->mutable_pos()->mutate_z(4);         // Set struct field.
    monster->mutable_inventory()->Mutate(0, 1);  // Set vector element.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We use the somewhat verbose term `mutate` instead of `set` to indicate that
this is a special use case, not to be confused with the default way of
constructing FlatBuffer data.

After the above mutations, you can send on the FlatBuffer to a new recipient
without any further work!

Note that any `mutate_` functions on tables return a bool, which is false
if the field we're trying to set isn't present in the buffer. Fields are not
present if they weren't set, or even if they happen to be equal to the
default value. For example, in the creation code above we set the `mana` field
to `150`, which is the default value, so it was never stored in the buffer.
Trying to call mutate_mana() on such data will return false, and the value won't
actually be modified!

One way to solve this is to call `ForceDefaults()` on a
`FlatBufferBuilder` to force all fields you set to actually be written. This
of course increases the size of the buffer somewhat, but this may be
acceptable for a mutable buffer.

Alternatively, you can use the more powerful reflection functionality:

### Reflection (& Resizing)

If the above ways of accessing a buffer are still too static for you, there is
experimental support for reflection in FlatBuffers, allowing you to read and
write data even if you don't know the exact format of a buffer, and even allows
you to change sizes of strings and vectors in-place.

The way this works is very elegant, there is actually a FlatBuffer schema that
describes schemas (!) which you can find in `reflection/reflection.fbs`.
The compiler `flatc` can write out any schemas it has just parsed as a binary
FlatBuffer, corresponding to this meta-schema.

Loading in one of these binary schemas at runtime allows you traverse any
FlatBuffer data that corresponds to it without knowing the exact format. You
can query what fields are present, and then read/write them after.

For convenient field manipulation, you can include the header
`flatbuffers/reflection.h` which includes both the generated code from the meta
schema, as well as a lot of helper functions.

And example of usage for the moment you can find in `test.cpp/ReflectionTest()`.

### Storing maps / dictionaries in a FlatBuffer

FlatBuffers doesn't support maps natively, but there is support to
emulate their behavior with vectors and binary search, which means you
can have fast lookups directly from a FlatBuffer without having to unpack
your data into a `std::map` or similar.

To use it:
-   Designate one of the fields in a table as they "key" field. You do this
    by setting the `key` attribute on this field, e.g.
    `name:string (key)`.
    You may only have one key field, and it must be of string or scalar type.
-   Write out tables of this type as usual, collect their offsets in an
    array or vector.
-   Instead of `CreateVector`, call `CreateVectorOfSortedTables`,
    which will first sort all offsets such that the tables they refer to
    are sorted by the key field, then serialize it.
-   Now when you're accessing the FlatBuffer, you can use `Vector::LookupByKey`
    instead of just `Vector::Get` to access elements of the vector, e.g.:
    `myvector->LookupByKey("Fred")`, which returns a pointer to the
    corresponding table type, or `nullptr` if not found.
    `LookupByKey` performs a binary search, so should have a similar speed to
    `std::map`, though may be faster because of better caching. `LookupByKey`
    only works if the vector has been sorted, it will likely not find elements
    if it hasn't been sorted.

### Direct memory access

As you can see from the above examples, all elements in a buffer are
accessed through generated accessors. This is because everything is
stored in little endian format on all platforms (the accessor
performs a swap operation on big endian machines), and also because
the layout of things is generally not known to the user.

For structs, layout is deterministic and guaranteed to be the same
accross platforms (scalars are aligned to their
own size, and structs themselves to their largest member), and you
are allowed to access this memory directly by using `sizeof()` and
`memcpy` on the pointer to a struct, or even an array of structs.

To compute offsets to sub-elements of a struct, make sure they
are a structs themselves, as then you can use the pointers to
figure out the offset without having to hardcode it. This is
handy for use of arrays of structs with calls like `glVertexAttribPointer`
in OpenGL or similar APIs.

It is important to note is that structs are still little endian on all
machines, so only use tricks like this if you can guarantee you're not
shipping on a big endian machine (an `assert(FLATBUFFERS_LITTLEENDIAN)`
would be wise).

### Access of untrusted buffers

The generated accessor functions access fields over offsets, which is
very quick. These offsets are not verified at run-time, so a malformed
buffer could cause a program to crash by accessing random memory.

When you're processing large amounts of data from a source you know (e.g.
your own generated data on disk), this is acceptable, but when reading
data from the network that can potentially have been modified by an
attacker, this is undesirable.

For this reason, you can optionally use a buffer verifier before you
access the data. This verifier will check all offsets, all sizes of
fields, and null termination of strings to ensure that when a buffer
is accessed, all reads will end up inside the buffer.

Each root type will have a verification function generated for it,
e.g. for `Monster`, you can call:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
	bool ok = VerifyMonsterBuffer(Verifier(buf, len));
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

if `ok` is true, the buffer is safe to read.

Besides untrusted data, this function may be useful to call in debug
mode, as extra insurance against data being corrupted somewhere along
the way.

While verifying a buffer isn't "free", it is typically faster than
a full traversal (since any scalar data is not actually touched),
and since it may cause the buffer to be brought into cache before
reading, the actual overhead may be even lower than expected.

In specialized cases where a denial of service attack is possible,
the verifier has two additional constructor arguments that allow
you to limit the nesting depth and total amount of tables the
verifier may encounter before declaring the buffer malformed. The default is
`Verifier(buf, len, 64 /* max depth */, 1000000, /* max tables */)` which
should be sufficient for most uses.

## Text & schema parsing

Using binary buffers with the generated header provides a super low
overhead use of FlatBuffer data. There are, however, times when you want
to use text formats, for example because it interacts better with source
control, or you want to give your users easy access to data.

Another reason might be that you already have a lot of data in JSON
format, or a tool that generates JSON, and if you can write a schema for
it, this will provide you an easy way to use that data directly.

(see the schema documentation for some specifics on the JSON format
accepted).

There are two ways to use text formats:

### Using the compiler as a conversion tool

This is the preferred path, as it doesn't require you to add any new
code to your program, and is maximally efficient since you can ship with
binary data. The disadvantage is that it is an extra step for your
users/developers to perform, though you might be able to automate it.

    flatc -b myschema.fbs mydata.json

This will generate the binary file `mydata_wire.bin` which can be loaded
as before.

### Making your program capable of loading text directly

This gives you maximum flexibility. You could even opt to support both,
i.e. check for both files, and regenerate the binary from text when
required, otherwise just load the binary.

This option is currently only available for C++, or Java through JNI.

As mentioned in the section "Building" above, this technique requires
you to link a few more files into your program, and you'll want to include
`flatbuffers/idl.h`.

Load text (either a schema or json) into an in-memory buffer (there is a
convenient `LoadFile()` utility function in `flatbuffers/util.h` if you
wish). Construct a parser:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    flatbuffers::Parser parser;
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Now you can parse any number of text files in sequence:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.cpp}
    parser.Parse(text_file.c_str());
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This works similarly to how the command-line compiler works: a sequence
of files parsed by the same `Parser` object allow later files to
reference definitions in earlier files. Typically this means you first
load a schema file (which populates `Parser` with definitions), followed
by one or more JSON files.

As optional argument to `Parse`, you may specify a null-terminated list of
include paths. If not specified, any include statements try to resolve from
the current directory.

If there were any parsing errors, `Parse` will return `false`, and
`Parser::err` contains a human readable error string with a line number
etc, which you should present to the creator of that file.

After each JSON file, the `Parser::fbb` member variable is the
`FlatBufferBuilder` that contains the binary buffer version of that
file, that you can access as described above.

`samples/sample_text.cpp` is a code sample showing the above operations.

### Threading

Reading a FlatBuffer does not touch any memory outside the original buffer,
and is entirely read-only (all const), so is safe to access from multiple
threads even without synchronisation primitives.

Creating a FlatBuffer is not thread safe. All state related to building
a FlatBuffer is contained in a FlatBufferBuilder instance, and no memory
outside of it is touched. To make this thread safe, either do not
share instances of FlatBufferBuilder between threads (recommended), or
manually wrap it in synchronisation primites. There's no automatic way to
accomplish this, by design, as we feel multithreaded construction
of a single buffer will be rare, and synchronisation overhead would be costly.
