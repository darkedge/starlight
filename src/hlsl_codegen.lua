-- Vertex specification
vertex = {"AppData", {
	{"float2", "uv", "TEXCOORD"},
	{"float3", "position", "POSITION"},
}}

-- Constant buffers
buffers = {}
buffers[1] = {"CBView", "b0", {
	{"matrix", "view"},
}}
buffers[2] = {"CBProjection", "b1", {
	{"matrix", "projection"},
}}
buffers[3] = {"CBModel", "b2", {
	{"matrix", "worldMatrix"},
}}

types = {}
types["matrix"] = "glm::mat4"
types["float"] = "float"
types["float2"] = "glm::vec2"
types["float3"] = "glm::vec3"
types["float4"] = "glm::vec4"

format = {}
format["float"] = "DXGI_FORMAT_R32_FLOAT"
format["float2"] = "DXGI_FORMAT_R32G32_FLOAT"
format["float3"] = "DXGI_FORMAT_R32G32B32_FLOAT"
format["float4"] = "DXGI_FORMAT_R32G32B32A32_FLOAT"

--
-- Generator
--

file = io.open("starlight_hlsl_generated.h", "w")
file:write("// This file is generated by ", arg[0],"\n\n")

file:write("#ifdef __cplusplus\n\n")

file:write("#include \"starlight_glm.h\"\n")
file:write("#include <d3d11.h>\n\n")

-- D3D11 Input Element Descriptor
file:write("D3D11_INPUT_ELEMENT_DESC g_", vertex[1], "[] =\n")
file:write("{", "\n")
for k,v in pairs(vertex[2]) do
	file:write("\t{ \"", v[3], "\", 0, ", format[v[1]], ", 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },\n")
end
file:write("};", "\n\n")

-- C++ structs
-- HLSL Input struct
file:write("struct ", vertex[1], "\n")
file:write("{\n")
for k,v in pairs(vertex[2]) do
	file:write("\t", types[v[1]], " ", v[2], ";\n")
end
file:write("};\n\n")

for k,v in pairs(buffers) do
	file:write("struct ", v[1], "\n")
	file:write("{\n")

	for k1,v1 in pairs(v[3]) do
		file:write("\t")
		file:write(types[v1[1]]) -- Type
		file:write(" ", v1[2], ";\n") -- Identifier
	end
	file:write("};\n\n")
end

file:write("#else\n\n")

-- HLSL Input struct
file:write("struct ", vertex[1], "\n")
file:write("{\n")
for k,v in pairs(vertex[2]) do
	file:write("\t", v[1], " ", v[2], " : ", v[3], ";\n")
end
file:write("};\n\n")

-- HLSL cbuffers
for k,v in pairs(buffers) do
	file:write("cbuffer ", v[1], " : register(", v[2], ")\n")
	file:write("{\n")

	for k1,v1 in pairs(v[3]) do
		file:write("\t")
		file:write(v1[1]) -- Type
		file:write(" ", v1[2], ";\n") -- Identifier
	end
	file:write("};\n\n")
end

file:write("#endif\n")

file:close()