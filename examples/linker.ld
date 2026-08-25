ENTRY(_start)

__stack_top = 0x10000;

SECTIONS
{
    . = 0x100;

    .text :
    {
        *(.text*)
    }
}
