#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool Contain(temp::Temp *tmp, temp::TempList *list){
  for(auto t : list->GetList()){
    if(t == tmp){
      return true;
    }
  }
  return false;
}

temp::TempList *Union(temp::TempList *left, temp::TempList *right){
  temp::TempList *res = new temp::TempList();
  for(auto tmp : left->GetList()){
    if(!Contain(tmp, res)){
      res->Append(tmp);
    }
  }
  for(auto tmp : right->GetList()){
    if(!Contain(tmp, res)){
      res->Append(tmp);
    }
  }

  return res;
}

temp::TempList *Sub(temp::TempList *left, temp::TempList *right){
  temp::TempList *res = new temp::TempList();
  for(auto tmp : left->GetList()){
    if(!Contain(tmp, right)){
      res->Append(tmp);
    }
  }

  return res;
}

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (!Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  //P157 algorithm
  printf("begin livemap\n");
  bool flag = true;
  while(flag){
    fg::FNodeListPtr nodes = flowgraph_->Nodes();
    std::list<fg::FNodePtr> node_list = nodes->GetList();
    for(auto node : node_list){
      temp::TempList *old_in = in_->Look(node);
      temp::TempList *old_out = out_->Look(node);
      in_->Set(node, Union(node->NodeInfo()->Use(), Sub(out_->Look(node), node->NodeInfo()->Def())));
      for(auto succ : (node->Succ())->GetList()){
        out_->Set(node, Union(out_->Look(node), in_->Look(succ)));
      }
      if(!(in_->Look(node) == old_in && out_->Look(node) == old_out)){
        flag = false;
      }
    }
  }
  printf("end livemap\n");
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  printf("begin interf graph\n");
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live