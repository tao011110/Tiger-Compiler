#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  auto *list = new assem::InstrList();
  // fs_ = fs_ - frame_->offset;
  for(auto stm : traces_->GetStmList()->GetList()){
    stm->Munch(*list, fs_);
  }

  assem_instr_ = std::make_unique<AssemInstr>(frame::procEntryExit2(list));
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*dst_) == typeid(tree::MemExp)){
    tree::MemExp *dst_mem = static_cast<MemExp*>(dst_);
    if(typeid(*(dst_mem->exp_)) == typeid(tree::BinopExp)){
      tree::BinopExp *dst_binop = static_cast<tree::BinopExp *>(dst_mem->exp_);
      if(dst_binop->op_ == tree::PLUS_OP && typeid(*dst_binop->right_) == typeid(tree::ConstExp)){
        /* MOVE(MEM(e1+i), e2) */
        tree::Exp *e1 = dst_binop->left_;
        tree::Exp *e2 = src_;
        e1->Munch(instr_list, fs);
        e2->Munch(instr_list, fs);
        instr_list.Append(new assem::MoveInstr("STORE", nullptr, nullptr));
      }
      else{
        if(dst_binop->op_ == tree::PLUS_OP && typeid(*dst_binop->left_) == typeid(tree::ConstExp)){
          /* MOVE(MEM(e1, e2+i)) */
          tree::Exp *e1 = dst_binop->right_;
          tree::Exp *e2 = src_;
          e1->Munch(instr_list, fs);
          e2->Munch(instr_list, fs);
          instr_list.Append(new assem::MoveInstr("STORE", nullptr, nullptr));
        }
      }
    }
    else{
      if(typeid(*src_) == typeid(tree::MemExp)){
        /* MOVE(MEM(e1), MEM(e2)) */
        tree::MemExp *src_mem = static_cast<tree::MemExp*>(src_);
        tree::Exp *e1 = dst_mem->exp_, *e2 = src_mem->exp_;
        e1->Munch(instr_list, fs);
        e2->Munch(instr_list, fs);
        instr_list.Append(new assem::MoveInstr("MOVEM", nullptr, nullptr));
      }
      else{
        /* MOVE(MEM(e1), e2) */
        tree::Exp *e1 = dst_mem->exp_, *e2 = src_;
        e1->Munch(instr_list, fs);
        e2->Munch(instr_list, fs);
        instr_list.Append(new assem::MoveInstr("STORE", nullptr, nullptr));
      }
    }
  }
  else{
    if(typeid(*dst_) == typeid(tree::TempExp)){
      /*  MOVE(TEMP~i, e2) */
      tree::Exp *e2 = src_;
      e2->Munch(instr_list, fs);
      instr_list.Append(new assem::MoveInstr("LOAD", nullptr, nullptr));
    }
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
}

} // namespace tree
