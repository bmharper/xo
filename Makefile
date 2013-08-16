MKDIR = mkdir -p
DLLSUFFIX = .a

BUILDDIR := build

NUDOM_SOURCES = \
	nudom/nuDefs.cpp \
	nudom/nuDoc.cpp \
	nudom/nuDomEl.cpp \
	nudom/nuEvent.cpp \
	nudom/nuLayout.cpp \
	nudom/nuMem.cpp \
	nudom/nuMsgLoop_Windows.cpp \
	nudom/nuPlatform.cpp \
	nudom/nuString.cpp \
	nudom/nuStringTable.cpp \
	nudom/nuStyle.cpp \
	nudom/nuStyleParser.cpp \
	nudom/nuSysWnd.cpp \
	nudom/nuDocGroup.cpp \
	nudom/nuDocGroup_Windows.cpp \
	nudom/Image/nuImage.cpp \
	nudom/Image/nuImageStore.cpp \
	nudom/Render/nuRenderer.cpp \
	nudom/Render/nuRenderGL.cpp \
	nudom/Render/nuRenderDoc.cpp \
	nudom/Render/nuRenderDomEl.cpp \
	nudom/Render/nuRenderStack.cpp \
	nudom/Render/nuStyleResolve.cpp \
	nudom/Text/nuTextCache.cpp \
	nudom/Text/nuTextDefs.cpp \
	dependencies/Panacea/Containers/queue.cpp \
	dependencies/Panacea/Platform/cpu.cpp \
	dependencies/Panacea/Platform/err.cpp \
	dependencies/Panacea/Platform/syncprims.cpp \
	dependencies/Panacea/Platform/thread.cpp \
	dependencies/Panacea/Strings/fmt.cpp \
	dependencies/glext.cpp

NUDOM_OBJECTS := $(addprefix $(BUILDDIR)/,$(NUDOM_SOURCES:.cpp=.o))

all: $(BUILDDIR)/nudom$(DLLSUFFIX)

Q ?=
E ?= @:

$(BUILDDIR):
	$(MKDIR) $(BUILDDIR)

$(BUILDDIR)/%.o: %.cpp
	@mkdir -p $(BUILDDIR)
	$(E) "CXX $<"
	$(Q) $(CXX) -c -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

$(BUILDDIR)/nudom$(DLLSUFFIX): $(NUDOM_OBJECTS)
	$(E) "AR $@"
	$(Q) $(AR) $@ $^
