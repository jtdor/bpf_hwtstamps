BPFTOOL ?= /usr/sbin/bpftool
CLANG ?= clang
CLANGXX ?= clang++
LLVM_STRIP ?= llvm-strip
VMLINUX ?= /sys/kernel/btf/vmlinux

LIBBPF_CFLAGS := $(shell pkg-config --cflags libbpf)

BPF_CFLAGS :=  \
	$(LIBBPF_CFLAGS) \
	-I.. \
	-O2 \
	-Wall \
	-Wextra \
	-g \
	-mcpu=v3 \
	-target bpf
CXX := $(CLANGXX)
CXXFLAGS := -O3
LDLIBS := $(LIBBPF_LIBS)

hwtstamps: CC=$(CXX)

all: hwtstamps hwtstamps.bpf.o

hwtstamps: hwtstamps.o

hwtstamps.bpf.o: hwtstamps.bpf.c
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@
	$(LLVM_STRIP) -g $@

hwtstamps.bpf.d: hwtstamps.bpf.c
	$(CLANG) -MM -MG $< -MF $@

hwtstamps.d: hwtstamps.cpp
	$(CXX) -M -MG  $< -MF $@

vmlinux.h: /sys/kernel/btf/vmlinux
	$(BPFTOOL) btf dump file $< format c > $@

/sys/fs/bpf:
	mount -t bpf none $@

/sys/fs/cgroup/hwtstamps:
	mkdir $@

clean:
	$(RM) hwtstamps hwtstamps.bpf.d hwtstamps.bpf.o hwtstamps.d hwtstamps.o vmlinux.h

detach_and_unload:
	-$(BPFTOOL) cgroup detach /sys/fs/cgroup/hwtstamps/ ingress pinned /sys/fs/bpf/hwtstamps/hwtstamps_ingress
	-$(BPFTOOL) cgroup detach /sys/fs/cgroup/hwtstamps/ sock_ops pinned /sys/fs/bpf/hwtstamps/hwtstamps_established
	-$(RM) -r /sys/fs/bpf/hwtstamps
	#-$(BPFTOOL) cgroup detach /sys/fs/cgroup/hwtstamps/ sock_create pinned /sys/fs/bpf/hwtstamps/hwtstamps_create

load_and_attach: hwtstamps.bpf.o | /sys/fs/bpf /sys/fs/cgroup/hwtstamps
	$(BPFTOOL) prog loadall hwtstamps.bpf.o /sys/fs/bpf/hwtstamps
	$(BPFTOOL) cgroup attach /sys/fs/cgroup/hwtstamps/ ingress pinned /sys/fs/bpf/hwtstamps/hwtstamps_ingress multi
	$(BPFTOOL) cgroup attach /sys/fs/cgroup/hwtstamps/ sock_ops pinned /sys/fs/bpf/hwtstamps/hwtstamps_established multi
	#$(BPFTOOL) cgroup attach /sys/fs/cgroup/hwtstamps/ sock_create pinned /sys/fs/bpf/hwtstamps/hwtstamps_create multi

ifneq ($(MAKECMDGOALS),clean)
-include hwtstamps.bpf.d hwtstamps.d
endif
