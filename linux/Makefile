EXTERNAL := external
BUILD_DIR := ./build

ENET := $(EXTERNAL)/enet-1.3.13
ENET_SRCS := $(shell find $(ENET) -name *.c)
ENET_OBJS := $(ENET_SRCS:$(ENET)/%.c=$(BUILD_DIR)/enet/%.o)

IMGUI := $(EXTERNAL)/imgui-1.49
IMGUI_SRCS := imgui.cpp imgui_demo.cpp imgui_draw.cpp examples/opengl3_example/imgui_impl_glfw_gl3.cpp
IMGUI_OBJS := $(IMGUI_SRCS:%.cpp=$(BUILD_DIR)/imgui/%.o)

CCFLAGS := -std=c++11
slInclude := . include external/sce_vectormath-master/include external/protobuf-2.6.1/src external/enet-1.3.13/include external/imgui-1.49
LINK := -lm -lX11 -lGL -lGLU

all: starlight.so starlight_linux assets

assets:
	cp $(IMGUI)/extra_fonts/*.ttf assets

ModdingApi.class: modding-api/src/ModdingApi.java
	javac -d . modding-api/src/ModdingApi.java

starlight_java_generated.h: ModdingApi.class
	javah -o starlight_java_generated.h ModdingApi

enet.a: $(ENET_OBJS)
	$(AR) -rs $@ $(ENET_OBJS)

$(BUILD_DIR)/enet/%.o: $(ENET)/%.c
	mkdir -p $(dir $@)
	$(CC) -DHAS_SOCKLEN_T -I$(ENET)/include -c $< -o $@

imgui.a: $(IMGUI_OBJS)
	$(AR) -rs $@ $(IMGUI_OBJS)

$(BUILD_DIR)/imgui/%.o: $(addprefix $(IMGUI)/,$(IMGUI_SRCS))
	mkdir -p $(dir $@)
	$(CC) -I$(IMGUI) -c $< -o $@

starlight.so: enet.a imgui.a starlight/*.cpp include/*.h starlight_java_generated.h
	$(CC) $(CCFLAGS) $(addprefix -I,$(slInclude)) -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux -shared -o $@ -fPIC starlight/starlight_ub.cpp

starlight_linux: enet.a imgui.a linux/*.cpp include/*.h
	$(CC) $(CCFLAGS) $(addprefix -I,$(slInclude)) -o $@ linux/starlight_linux_ub.cpp $(LINK)

clean:
	rm -f assets/*.ttf
	rm -f starlight_java_generated.h
	rm -rf build
	rm -f starlight.so
	rm -f ModdingApi.class
	rm -f *.a