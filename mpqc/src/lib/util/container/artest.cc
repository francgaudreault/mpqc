//
// artest.cc
//
// Copyright (C) 1996 Limit Point Systems, Inc.
//
// Author: Curtis Janssen <cljanss@limitpt.com>
// Maintainer: LPS
//
// This file is part of the SC Toolkit.
//
// The SC Toolkit is free software; you can redistribute it and/or modify
// it under the terms of the GNU Library General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// The SC Toolkit is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with the SC Toolkit; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
//
// The U.S. Government is granted a limited license as per AL 91-7.
//

#include <iostream.h>
#include <util/container/array.h>
#include <util/state/state.h>

#ifdef __GNUG__
template class Array<int>;
template class SSBArray<int>;
template class Array2<int>;
template class SSBArray2<int>;
#endif

main()
{
  int i;
  SSBArray<int> a(10);

  for (i=0; i<10; i++) {
      a(i) = i;
    }

  for (i=0; i<10; i++) {
      cout << "a(" << i << ") = " << a(i) << endl;
    }

  ///////////////
  int j;
  SSBArray2<int> b(3,3);

  for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
          b(i,j) = i + j;
        }
    }

  for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
          cout << "b(" << i << "," << j << ") = " << b(i,j) << endl;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: c++
// c-file-style: "CLJ"
// End:
