#ifndef __ENDIANESS_HANDLER__

  /* The following is based on xorg/xserver/GL/glx/glxbyteorder.h
   * which is (c) IBM Corp. 2006,2007 and originally licensed under the following
   * BSD-license. All modifications in navit are licensed under the GNU GPL as
   * described in file "COPYRIGHT".
   * --------------------------
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, THE AUTHORS, AND/OR THEIR SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
  #define le16_to_cpu(x)	(x)
  #define le32_to_cpu(x)	(x)
  #define cpu_to_le16(x)	(x)
  #define cpu_to_le16(x)	(x)

#define __ENDIANESS_HANDLER__
#endif

