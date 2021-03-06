//===---------------------------------------------------------------------===//
//
//                             The ONNC Project
//
// Copyright(c) 2018, The ONNC Team
//
// This file is part of the ONNC Project and is distributed under
// 3-clause BSD license (https://opensource.org/licenses/BSD-3-Clause)
//
// See LICENSE.TXT for details.
//
//===---------------------------------------------------------------------===//
#include "TGBackend.h"
#include "BM168x/BM168xBackend.h"
#include "BM188x/BM188xBackend.h"
#include "BuildMemOpndPass.h"
#include "LinearScanAllocPass.h"
#include "TG.h"
#include "TargetInfo/TGTargetInfo.h"
#include "TargetLowering.h"
#include <algorithm>
#include <onnc/Analysis/UpdateGraphOutputSize.h>
#include <onnc/IR/ONNCModulePrinter.h>
#include <onnc/Target/TargetRegistry.h>
#include <onnc/Transforms/RemoveTrainingNodes.h>

using namespace onnc;

//===----------------------------------------------------------------------===//
// TGBackend
//===----------------------------------------------------------------------===//
TGBackend::TGBackend(TargetLowering *pTLI, TGCodeEmitter *pCE,
                     Instructions& pInsns, const TargetOptions &pOptions)
    : DLATargetBackend(pOptions), m_Instructions(pInsns), m_pTLI(pTLI),
      m_pCE(pCE)
{
  m_ReplaceTargetLower = nullptr;
}

TGBackend::~TGBackend()
{
  m_Instructions.clear();
  for (auto &memOp : m_MemOperands) {
    delete (memOp);
  }
  delete m_pTLI;
  delete m_pCE;
}

void TGBackend::addTensorSel(PassManager &pPM)
{
  // IR level pass
  pPM.add(CreateRemoveTrainingNodesPass());
  pPM.add(CreateUpdateGraphOutputSizePass());
  pPM.add(createONNXFuseOptPass(this));
  if (options().shouldPrintBeforeTensorSel())
    pPM.add(createONNCModulePrinterPass());
  pPM.add(createTargetLoweringPass(this));
  return;
}

void TGBackend::addMemAlloc(PassManager &pPM)
{
  pPM.add(CreateGlobalMemAllocPass(this));
  pPM.add(CreateBuildMemOpndPass());
  pPM.add(CreateLinearScanAllocPass(this));
}

void TGBackend::addCodeEmit(PassManager &pPM, const Path &pOutput)
{
  pPM.add(CreateTGCodeEmitPass(this, pOutput.native()));
}

bool TGBackend::isNativeTensorType(xTensorProtoDataType pType)
{
  return true;
}

size_t TGBackend::sizeOfTensorType(xTensorProtoDataType pType)
{
  if (!isNativeTensorType(pType)) {
    std::cerr << "unsupported pType " << TensorProto_DataType_Name(pType)
              << " in " << getBackendName() << "\n";
    return 0;
  }
  switch (pType) {
  case xValueType::kFloat:
    return sizeof(float);
  case xValueType::kInt8:
    return sizeof(int8_t);
  case xValueType::kInt16:
    return sizeof(int16_t);
  default:
    assert(0 && "unimplemented size of pType!");
  }
  return 0;
}

std::string TGBackend::getCtable(const Module &pModule)
{
  auto &meta_data = pModule.getMetaData();
  if (meta_data.find(getCtableName()) == meta_data.end())
    return std::string();
  return meta_data.at(getCtableName());
}

void TGBackend::setCtableProto(const std::string &pTextString)
{
  if (getCtableName() != "") {
    // implemnt this function when target has ctable
    assert(0);
  }
}

MemOperand *TGBackend::getMemOperand(const xValue *pValue,
                                     MemType pMemType, const std::string &pName)
{
  std::string name = pName;
  if (pName.empty())
    name = pValue->uniqueName();
  auto it =
      std::find_if(m_MemOperands.begin(), m_MemOperands.end(),
                   [&](const auto &pElem) { return pElem->m_Name == name; });
  if (it != m_MemOperands.end()) {
    return *it;
  }
  MemOperand *memOp = new MemOperand(name, pValue, pMemType);
  m_MemOperands.push_back(memOp);
  return memOp;
}

onnc::ComputeMemOperand* TGBackend::getMemOpndByValue(const onnc::Value* pVal)
{
  auto it = m_ValMemOpndMap.find(pVal);
  return it != m_ValMemOpndMap.end() ? it->second : nullptr;
}

//===----------------------------------------------------------------------===//
// Non member functions
//===----------------------------------------------------------------------===//
TargetBackend *CreateTGBM1680Backend(const TargetOptions &pOptions)
{
  static TGBackend::Instructions insns;
  return new BM1680Backend(insns, pOptions);
}

TargetBackend *CreateTGBM1682Backend(const TargetOptions &pOptions)
{
  static TGBackend::Instructions insns;
  return new BM1682Backend(insns, pOptions);
}

TargetBackend *CreateTGBM1880Backend(const TargetOptions &pOptions)
{
  static TGBackend::Instructions insns;
  return new BM1880Backend(insns, pOptions);
}

extern "C" void InitializeSophonONNCBackend()
{
  onnc::TargetRegistry::RegisterTargetBackend(getTheTGBM1680Target(),
                                              CreateTGBM1680Backend);
  onnc::TargetRegistry::RegisterTargetBackend(getTheTGBM1682Target(),
                                              CreateTGBM1682Backend);
  onnc::TargetRegistry::RegisterTargetBackend(getTheTGBM1880Target(),
                                              CreateTGBM1880Backend);
}
