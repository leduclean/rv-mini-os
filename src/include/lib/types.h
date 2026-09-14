/*
 * types.h
 *
 * Copyright (C) 2002 Simon Nieuviarts
 *
 * System types.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __RISCV64_TYPES_H__
#define __RISCV64_TYPES_H__

/*
 * size_t and ptrdiff_t come from the compiler's freestanding <stddef.h>.
 * ssize_t is POSIX, so GCC does not provide it: it is the signed counterpart
 * of size_t, which __PTRDIFF_TYPE__ gives us on every sane target.
 */
typedef __PTRDIFF_TYPE__ ssize_t;

#endif /* __RISCV64_TYPES_H__ */
