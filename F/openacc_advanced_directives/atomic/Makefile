F90  = nvfortran
RM  = rm -f

FFLAGS    = -fast -mp -acc -gpu=cc90,mem:separate -Minfo=accel
LDFLAGS   = 

SRCS   = main.f90
TARGET = run
DISTTARGET = $(TARGET)_1.0.0

OBJS += $(filter %.o,$(SRCS:%.f90=%.o))


.PHONY: all
all : $(TARGET)

$(TARGET) : $(OBJS)
	$(F90) $(FFLAGS) $(TARGET_ARCH) $(OBJS) -o $@ $(LDFLAGS)

%.o : %.f90
	$(F90) $(FFLAGS) $(TARGET_ARCH) -c $<


.PHONY: clean
clean :
	$(RM) $(TARGET)
	$(RM) $(OBJS)
	$(RM) *.mod
	$(RM) *~


