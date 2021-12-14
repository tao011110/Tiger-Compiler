#include "tiger/liveness/flowgraph.h"

extern frame::RegManager *reg_manager;

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  std::list<assem::Instr *> il = instr_list_->GetList();
  FNode *prev = nullptr;
  for(auto instr : il){
    FNode *cur = flowgraph_->NewNode(instr);
    if(prev){
      flowgraph_->AddEdge(prev, cur);
    }
    if(((assem::OperInstr *)instr)->jumps_ != nullptr){
      printf("is jump\n");
      prev = nullptr;
      continue;
    }
    if(((assem::LabelInstr *)instr)->label_ != nullptr){
      printf("is label instr\n");
      label_map_->Enter(((assem::LabelInstr *)instr)->label_, cur);
    }
    prev = cur;
  }
  FNodeListPtr nodes = flowgraph_->Nodes();
  std::list<FNodePtr> node_list = nodes->GetList();
  for(auto node: node_list){
    assem::Targets *jumps = (((assem::OperInstr *)(node->NodeInfo()))->jumps_;
    if(jumps != nullptr){
      int size = jumps->labels_->size();
      for(int i = 0; i < size; i++){
        FNode *lable_node = label_map_->Look(jumps->labels_->at(i));
        flowgraph_->AddEdge(node, lable_node);
      }
    }
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  temp::TempList *result = nullptr;
  std::list<temp::Temp *> dst_list = dst_->GetList();
  for(auto tmp : dst_list){
    if(tmp == reg_manager->StackPointer()){
      continue;
    }
    if(!result){
      result = new temp::TempList(tmp);
    }
    else{
      result->Append(tmp);
    }
  }

  return result;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  temp::TempList *result = nullptr;
  std::list<temp::Temp *> dst_list = dst_->GetList();
  for(auto tmp : dst_list){
    if(tmp == reg_manager->StackPointer()){
      continue;
    }
    if(!result){
      result = new temp::TempList(tmp);
    }
    else{
      result->Append(tmp);
    }
  }

  return result;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return nullptr;
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  temp::TempList *result = nullptr;
  std::list<temp::Temp *> src_list = src_->GetList();
  for(auto tmp : src_list){
    if(tmp == reg_manager->StackPointer()){
      continue;
    }
    if(!result){
      result = new temp::TempList(tmp);
    }
    else{
      result->Append(tmp);
    }
  }

  return result;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  temp::TempList *result = nullptr;
  std::list<temp::Temp *> src_list = src_->GetList();
  for(auto tmp : src_list){
    if(tmp == reg_manager->StackPointer()){
      continue;
    }
    if(!result){
      result = new temp::TempList(tmp);
    }
    else{
      result->Append(tmp);
    }
  }

  return result;
}
} // namespace assem
