#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool Contain(temp::Temp *tmp, temp::TempList *list){
  if(list){
    for(auto t : list->GetList()){
      if(t == tmp){
        return true;
      }
    }
  }
  return false;
}

temp::TempList *Union(temp::TempList *left, temp::TempList *right){
  // printf("union\n");
  temp::TempList *res = new temp::TempList();
  if(left){
    for(auto tmp : left->GetList()){
      if(!Contain(tmp, res)){
        res->Append(tmp);
      }
    }
  }
  if(right){
    for(auto tmp : right->GetList()){
      if(!Contain(tmp, res)){
        res->Append(tmp);
      }
    }
  }

  return res;
}

temp::TempList *Sub(temp::TempList *left, temp::TempList *right){
  temp::TempList *res = new temp::TempList();
  if(left){
    for(auto tmp : left->GetList()){
      if(!Contain(tmp, right)){
        res->Append(tmp);
      }
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
  // printf("1111\n");
  for (auto move : move_list_) {
  // printf("122\n");
    res->move_list_.push_back(move);
  }
  // printf("222\n");
  for (auto move : list->GetList()) {
  // printf("2333\n");
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  // printf("3333\n");
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

bool LiveGraphFactory::isSame(temp::TempList *left, temp::TempList *right){
  auto left_it = left->GetList().begin();
  if(left->GetList().size() != right->GetList().size()){
    return false;
  }
  for(; left_it != left->GetList().end(); left_it++){
    auto right_it = right->GetList().begin();
    for(; right_it != right->GetList().end(); right_it++){
      if((*left_it)->Int() == (*right_it)->Int()){
        break;
      }
    }
    if(right_it == right->GetList().end()){
      return false;
    }
  }
  return true;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */
  //P157 algorithm
  printf("begin livemap\n");
  bool flag = true;
  printf("nodelist size is %d", int(flowgraph_->Nodes()->GetList().size()));
  for(auto node : flowgraph_->Nodes()->GetList()){
    in_.get()->Enter(node, new temp::TempList());
    out_.get()->Enter(node, new temp::TempList());
  }
  while(flag){
    // printf("while\n");
    flag = false;
    fg::FNodeListPtr nodes = flowgraph_->Nodes();
    std::list<fg::FNodePtr> node_list = nodes->GetList();
    for(auto node : node_list){
      temp::TempList *old_in = in_.get()->Look(node);
      temp::TempList *old_out = out_.get()->Look(node);
      temp::TempList *new_in = Union(node->NodeInfo()->Use(), Sub(out_.get()->Look(node), node->NodeInfo()->Def()));

      in_.get()->Set(node, new_in);
      // printf("why %d but %d\n", int(in_.get()->Look(node)->GetList().size()), int(new_in->GetList().size()));
      // printf("woc %d\n", int(old_in->GetList().size()));

      for(auto succ : (node->Succ())->GetList()){
        out_.get()->Set(node, Union(out_.get()->Look(node), in_.get()->Look(succ)));
      }
      // if(!out_.get()->Look(node)){
      //   printf("new out is null\n");
      // }
      if(!(isSame(in_.get()->Look(node), old_in) && isSame(out_.get()->Look(node), old_out))){
        // printf("yes?\n");
        flag = true;
      }
      // else{
      //   printf("no?\n");
      // }
    }
  }
  printf("end livemap\n");
}

INode *LiveGraphFactory::getNode(temp::Temp *temp){
  // printf("try to get %d\n", temp->Int());
  for(auto *node : live_graph_.interf_graph->Nodes()->GetList()){
    if(node->NodeInfo() == temp){
      return node;
    }
  }
  return live_graph_.interf_graph->NewNode(temp);
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  printf("begin interf graph\n");
  //TODO 
  for(auto *temp1 : reg_manager->Registers()->GetList()){
    for(auto *temp2 : reg_manager->Registers()->GetList()){
      auto m = getNode(temp1);
      auto n = getNode(temp2);
      if(m != n){
        live_graph_.interf_graph->AddEdge(m, n);
        live_graph_.interf_graph->AddEdge(n, m);
      }
    }
  }
  printf("end1\n");

  for(auto node : flowgraph_->Nodes()->GetList()){
    printf("for begin\n");
    temp::TempList *def = node->NodeInfo()->Def();
    temp::TempList *use = node->NodeInfo()->Use();
    if(typeid(*(node->NodeInfo())) == typeid(assem::MoveInstr)){
      printf("node MoveInstr assem is %s\n", ((assem::MoveInstr*)(node->NodeInfo()))->assem_.c_str());
      if(use->GetList().size() == 0 || def->GetList().size() == 0){
        continue;
      }
      auto src = getNode(*(use->GetList().begin()));
      auto dst = getNode(*(def->GetList().begin()));
      live_graph_.moves->Append(src, dst);
      if(out_->Look(node)){
        auto temp_list = out_->Look(node)->GetList();
        for(auto temp : temp_list){
          if(temp == *(use->GetList().begin())){
            continue;
          }
          auto out = getNode(temp);
          if(dst != out){
            live_graph_.interf_graph->AddEdge(dst, out);
            live_graph_.interf_graph->AddEdge(out, dst);
          }
        }
      }
    }
    else{
      if(typeid(*(node->NodeInfo())) == typeid(assem::OperInstr)){
        printf("node OperInstr assem is %s\n", ((assem::OperInstr*)(node->NodeInfo()))->assem_.c_str());
      }
      if(out_->Look(node)){
        if(!def){
          printf("!def\n");
          continue;
        }
        for(auto d :def->GetList()){
          auto temp_list = out_->Look(node)->GetList();
          for(auto temp : temp_list){
            auto dst = getNode(d);
            auto out = getNode(temp);
            if(dst != out){
              live_graph_.interf_graph->AddEdge(dst, out);
              live_graph_.interf_graph->AddEdge(out, dst);
            }
          }
        }
      }
    }
  }
  printf("end interf graph\n");
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
