NovaOS Recovery — Stage-4C verified foundation

/Recovery is a dedicated 24 MiB GPT partition: 20 MiB ext4 data plus an appended
dm-verity tree. Its descriptor is authenticated by the content root carried inside
the firmware-authenticated kernel/initramfs chain. novad mounts only the verified
/dev/mapper/nova-recovery mapping read-only,nosuid,nodev,noexec.

Recovery descriptor/block verification failure is fail-closed. Stage-4C also requires
UEFI Secure Boot, TPM anti-rollback state and the measured-boot gate before normal READY.
A standalone firmware-bootable graphical recovery environment remains future work.
