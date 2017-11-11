CC  = gcc
AR  = ar

CFLAGS = -g
LFLAGS = -lpthread

TGT = RRdriver

OBJS =  pet_thread.o \
	pet_thread_hw.o \
	RRdriver.o     \
	pet_hashtable.o \
	pet_log.o \


build = \
	@if [ -z "$V" ]; then \
		echo '	[$1]	$@'; \
		$2; \
	else \
		echo '$2'; \
		$2; \
	fi

% : %.o
	$(call build,LINK,$(CC) $(CFLAGS) $(OBJS)  -o $@ $(LFLAGS))

%.o : %.c 
	$(call build,CC,$(CC) $(CFLAGS) -c $< -o $@)

%.o : %.S 
	$(call build,CC,$(CC) $(CFLAGS) -c $< -o $@)

%.a : %.o
	$(call build,AR,$(AR) rcs $@ $^)


all: RRdriver
RRdriver : $(OBJS)

clean: 
	rm -f *.o $(OBJS) $(TGT)

