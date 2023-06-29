#include "vmlinux.h"

#include <bpf/bpf_helpers.h>

char _license[] SEC("license") = "GPL";

#define SOL_SOCKET 1

#define SO_TIMESTAMPING_NEW 65

struct
{
	__uint(type, BPF_MAP_TYPE_SK_STORAGE);
	__uint(map_flags, BPF_F_NO_PREALLOC);
	__type(key, int);
	__type(value, u64);
} map_prev_hwtstamps SEC(".maps");

struct
{
	__uint(type, BPF_MAP_TYPE_SK_STORAGE);
	__uint(map_flags, BPF_F_NO_PREALLOC);
	__type(key, int);
	__type(value, u64);
} map_prev_tstamps SEC(".maps");

SEC("sockops")
int hwtstamps_established(struct bpf_sock_ops* skops)
{
	switch (skops->op) {
		case BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB:
		case BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB: {
#if 1
			int val = SOF_TIMESTAMPING_RX_HARDWARE;
			bpf_setsockopt(
			    skops, SOL_SOCKET, SO_TIMESTAMPING_NEW, &val, sizeof(val));
#endif
		}
	}

	return 1;
}

SEC("cgroup_skb/ingress")
int hwtstamps_ingress(struct __sk_buff* skb)
{
	struct bpf_sock* sk = skb->sk;
	if (!sk) {
		return 1;
	}

	u64 hwts = skb->hwtstamp;
	u64* prev_hwts = bpf_sk_storage_get(
	    &map_prev_hwtstamps, sk, NULL, BPF_SK_STORAGE_GET_F_CREATE);
	if (hwts && prev_hwts) {
		if (*prev_hwts) {
			u64 diff = hwts - *prev_hwts;
			bpf_printk("hwtstamp=%lu diff=%lu", hwts, diff);
		} else {
			bpf_printk("hwtstamp=%lu", hwts);
		}

		*prev_hwts = hwts;
	}

	u64 ts = skb->tstamp;
	u64* prev_ts = bpf_sk_storage_get(
	    &map_prev_tstamps, sk, NULL, BPF_SK_STORAGE_GET_F_CREATE);
	if (ts && prev_ts) {
		if (*prev_ts) {
			u64 diff = ts - *prev_ts;
			bpf_printk("tstamp=%lu diff=%lu", ts, diff);
		} else {
			bpf_printk("tstamp=%lu", ts);
		}

		*prev_ts = ts;
	}

	return 1;
}
