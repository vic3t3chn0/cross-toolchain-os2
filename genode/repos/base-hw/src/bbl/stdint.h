/**
 * \brief  Base types
 * \author Sebastian Sumpf
 * \date   2017-08-24
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _STDINT_H_
#define _STDINT_H_

typedef unsigned long      uintptr_t;

typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned           uint32_t;
typedef unsigned long long uint64_t;
typedef signed long long   int64_t;

typedef signed long        ssize_t;
typedef unsigned long      size_t;

#endif /* _STDINT_H_ */
