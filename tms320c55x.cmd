/******************************************************************************/
/*									      */
/******************************************************************************/
-c

/*-l rts55x.lib*/
/* -stack 0x200 */			/* PRIMARY STACK SIZE */
/* -sysstack 0x100 */			/* SECONDARY STACK SIZE */
/* -heap  0x2000 */			/* HEAP AREA SIZE */

/* -u _Reset */				/* RESET VECTOR */

/* SPECIFY THE SECTIONS ALLOCATION INTO MEMORY */

MEMORY
{
 PAGE 0:

  MMR     (R): origin = 0x000000, length = 0x0000c0  /* MMRs */
/* 0x0000c0 l=0x000040 not used*/
  DARAM0  (RWIX): origin = 0x000100, length = 0x00FF00  /*  32Kb - MMRs */
  SARAM0  (RWIX): origin = 0x010000, length = 0x010000  /* 192Kb */
  SARAM1  (RWIX): origin = 0x020000, length = 0x010000  /* 192Kb */
  SARAM2  (RWIX): origin = 0x030000, length = 0x010000   /* 192Kb */

  PDROM   (RIX) : origin = 0xff8000, length = 0x008000  /*  32Kb */

  PAGE 2:  /* -------- 64K-word I/O Address Space -------- */

  IOPORT  (RWI) : origin = 0x000000, length = 0x020000
}
 
/* SPECIFY THE SECTIONS ALLOCATION INTO MEMORY */

SECTIONS
{
   .text      >> SARAM0|DARAM0,  align = 4

   .bss       >> SARAM1|SARAM2,  align = 1024	/* Global & static vars        */
   .data      > SARAM1,  align = 4		/* Initialized vars            */
   .const     > SARAM1,  align = 4		/* Constant data               */
   .sysmem    > SARAM1,  align = 4		/* Dynamic memory (malloc)     */
   .switch    > SARAM1,  align = 4		/* Switch statement tables     */
   .cinit     > SARAM1,  align = 4		/* Auto-initialization tables  */
   .pinit     > SARAM1				/* Initialization fn tables    */
   .cio       > SARAM1, align = 4		/* C I/O buffers               */

   .main_stack > SARAM1

   vectors: 0x000100			/* Interrupt vectors           */

   .ioport     >  IOPORT PAGE 2		/* Global & static ioport vars */
}
