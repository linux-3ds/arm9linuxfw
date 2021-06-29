#include "asm.h"

.arm
.align 2

.global vdev_reset_stub
.type vdev_reset_stub, %function

.global vdev_cfg_read_stub
.type vdev_cfg_read_stub, %function

.global vdev_cfg_write_stub
.type vdev_cfg_write_stub, %function

.global vdev_process_vqueue_stub
.type vdev_process_vqueue_stub, %function

@ Very simple virtDev operation stubs
vdev_cfg_read_stub:
	mov r0, #0

vdev_reset_stub:
vdev_cfg_write_stub:
vdev_process_vqueue_stub:
	bx lr
