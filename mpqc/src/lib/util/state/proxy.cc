
#ifdef __GNUG__
#pragma implementation
#endif

#include <util/state/state.h>
#include <util/state/proxy.h>
#include <util/keyval/keyval.h>

#define CLASSNAME SavableStateProxy
#define PARENTS public DescribedClassProxy
#define HAVE_KEYVAL_CTOR
#include <util/class/classi.h>
void *
SavableStateProxy::_castdown(const ClassDesc*cd)
{
  void* casts[1];
  casts[0] =  DescribedClassProxy::_castdown(cd) ;
  return do_castdowns(casts,cd);
}

SavableStateProxy::SavableStateProxy(const RefKeyVal &keyval)
{
  char *filename = keyval->pcharvalue("file");
  if (filename) {
      StateInBinXDR si(filename);
      object_.restore_state(si);
      delete[] filename;
    }
}

RefDescribedClass
SavableStateProxy::object()
{
  return object_;
}

