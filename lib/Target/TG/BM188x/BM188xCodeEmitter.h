#ifndef BM188X_CODE_EMITTER_H
#define BM188X_CODE_EMITTER_H
#include "BM188xBackend.h"
#include "TGCodeEmitter.h"
#include <memory>
#include <onnc/Support/Path.h>
#include <vector>

namespace bmnet {

class BM1880BackendContext;

} // namespace bmnet

namespace onnc {

class BM1880Backend;

class BM188xCodeEmitter : public TGCodeEmitter
{
public:
  BM188xCodeEmitter(BM1880Backend *pBackend);

  virtual ~BM188xCodeEmitter() = default;

  void encodeInstructions(const Path &pOutputPath) override;

private:
  void *m_bmkernelHandle;
  BM1880Backend *m_Backend;
};

class bm1880_kernel
{
public:
  static bm1880_kernel &getInstance()
  {
    static bm1880_kernel instance; // Guaranteed to be destroyed.
                                   // Instantiated on first use.
    return instance;
  }

public:
  bmnet::BM1880BackendContext *m_Ctx;

private:
  bm1880_kernel(bm1880_kernel const &) = delete;

  void operator=(bm1880_kernel const &) = delete;

  bm1880_kernel() = default;

  ~bm1880_kernel() = default;
};

} // namespace onnc

#endif