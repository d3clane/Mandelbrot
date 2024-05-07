CXX = g++
ASM = nasm
CXXFLAGS = -D _DEBUG -ggdb3 -std=c++17 -O2 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations	  \
		   -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts 		  \
		   -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal      \
		   -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op \
		   -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self \
		   -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel 		  \
		   -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods 				  \
		   -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand 		  \
		   -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix   \
		   -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs 			  \
		   -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow 	  \
		   -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie  \
		   -fPIE -Werror=vla -lsfml-graphics -lsfml-window -lsfml-system -D TIME_MEASURE			  \
		   -D TIME_MEASURE_PIXELS_SETTIN -D TIME_MEASURE_EXTRA_VAR -mavx2

HOME = $(shell pwd)
CXXFLAGS += -I $(HOME)

PROGRAMDIR = build/bin
TARGET1 = testNoAvx
TARGET2 = testAvx
TARGET3 = testNoAvxArrays
OBJECTDIR = build

DOXYFILE = Others/Doxyfile

HEADERS  = 

FILES1CPP = NoAvx.cpp
FILES1ASM = GetTimeStampCounter.s
FILES2CPP = Avx.cpp
FILES2ASM = GetTimeStampCounter.s
FILES3CPP = NoAvxArrays.cpp
FILES3ASM = GetTimeStampCounter.s

objects1  = $(FILES1CPP:%.cpp=$(OBJECTDIR)/%.o)
objects1 += $(FILES1ASM:%.s=$(OBJECTDIR)/%.o)

objects2  = $(FILES2CPP:%.cpp=$(OBJECTDIR)/%.o)
objects2 += $(FILES2ASM:%.s=$(OBJECTDIR)/%.o)

objects3  = $(FILES3CPP:%.cpp=$(OBJECTDIR)/%.o)
objects3 += $(FILES3ASM:%.s=$(OBJECTDIR)/%.o)

.PHONY: all docs clean buildDirs

all: $(PROGRAMDIR)/$(TARGET1) $(PROGRAMDIR)/$(TARGET2) $(PROGRAMDIR)/$(TARGET3)

$(PROGRAMDIR)/$(TARGET1): $(objects1)
	$(CXX) $^ -o $(PROGRAMDIR)/$(TARGET1) $(CXXFLAGS)

$(PROGRAMDIR)/$(TARGET2): $(objects2)
	$(CXX) $^ -o $(PROGRAMDIR)/$(TARGET2) $(CXXFLAGS)

$(PROGRAMDIR)/$(TARGET3): $(objects3)
	$(CXX) $^ -o $(PROGRAMDIR)/$(TARGET3) $(CXXFLAGS)

$(OBJECTDIR)/%.o : %.cpp $(HEADERS)
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

$(OBJECTDIR)/%.o : %.s
	$(ASM) -f elf64 $< -o $@

docs: 
	doxygen $(DOXYFILE)

clean:
	rm -rf $(OBJECTDIR)/*.o

buildDirs:
	mkdir $(OBJECTDIR)
	mkdir $(PROGRAMDIR)