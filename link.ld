OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(__start)

MEMORY
{
  AHBRAM  (RWX) : ORIGIN = 0x08080000, LENGTH = 64K
  VECTOR  (RWX) : ORIGIN = 0x08000000, LENGTH = 64
  ITCM    (RWX) : ORIGIN = 0x01FF8000, LENGTH = 32K
  DTCM    (RW)  : ORIGIN = 0x40000000, LENGTH = 16K
}

SECTIONS
{
    .bootstrap : ALIGN(16)
    {
        __bootstrap_start = ABSOLUTE(.);
        KEEP(*(.bootstrap))
        . = ALIGN(16);
    } >AHBRAM

    /* .vector, .text and .data get relocated on boot */
    .vector : ALIGN(16)
    {
        __vector_lma = LOADADDR(.vector);
        __vector_s = ABSOLUTE(.);
        KEEP(*(.vector))
        . = ALIGN(16);
        __vector_e = ABSOLUTE(.);
    } >VECTOR AT>AHBRAM

    .text : ALIGN(16)
    {
        __text_lma = LOADADDR(.text);
        __text_s = ABSOLUTE(.);
        KEEP(*(.text.vectors))
        *(.text*)
        . = ALIGN(16);
        __text_e = ABSOLUTE(.);
    } >ITCM AT>AHBRAM

    .data : ALIGN(16)
    {
        __data_lma = LOADADDR(.data);
        __data_s = ABSOLUTE(.);

        *(.data*)
        *(.rodata*)

        . = ALIGN(16);

        __vdev_s = ABSOLUTE(.);
        KEEP(*(.vdev_list));
        __vdev_cnt = (ABSOLUTE(.) - __vdev_s) / 4;

        . = ALIGN(16);
        __data_e = ABSOLUTE(.);
    } >DTCM AT>AHBRAM

    .bss (NOLOAD) : ALIGN(16)
    {
        __bss_s = ABSOLUTE(.);
        *(.bss*)
        . = ALIGN(16);
        __bss_e = ABSOLUTE(.);
    } >DTCM
}
