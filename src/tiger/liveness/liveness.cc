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
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
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

      for(auto succ : (node->Succ())->GetList()){
        out_.get()->Set(node, Union(out_.get()->Look(node), in_.get()->Look(succ)));
      }
      if(!(isSame(in_.get()->Look(node), old_in) && isSame(out_.get()->Look(node), old_out))){
        // printf("yes?\n");
        flag = true;
      }
    }
  }
  printf("end livemap\n");
}

INode *LiveGraphFactory::getNode(temp::Temp *temp){
  // printf("try to get %d\n", temp->Int());
  if(!temp_node_map_->Look(temp)){
    live::INodePtr node = live_graph_.interf_graph->NewNode(temp);
    temp_node_map_->Enter(temp, node);
    return node;
  }
  return temp_node_map_->Look(temp);
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  printf("begin interf graph\n");
  //TODO 
  for(auto temp : reg_manager->Registers()->GetList()){
    auto node = live_graph_.interf_graph->NewNode(temp);
    temp_node_map_->Enter(node->NodeInfo(), node);
  }
  for(auto *temp1 : reg_manager->Registers()->GetList()){
    for(auto *temp2 : reg_manager->Registers()->GetList()){
      if(temp1 != temp2){
        auto m = getNode(temp1);
        auto n = getNode(temp2);
        live_graph_.interf_graph->AddEdge(m, n);
        // live_graph_.interf_graph->AddEdge(n, m);
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
      for(auto d : def->GetList()){
        auto temp_list = out_->Look(node)->GetList();
        for(auto temp : temp_list){
          if(Contain(temp, use)){
            continue;
          }
          if(d == reg_manager->StackPointer() || temp == reg_manager->StackPointer()){
            continue;
          }
          live_graph_.interf_graph->AddEdge(getNode(d), getNode(temp));
          live_graph_.interf_graph->AddEdge(getNode(temp), getNode(d));
        }

        for(auto u : use->GetList()){
          if(d == reg_manager->StackPointer() || u == reg_manager->StackPointer()){
            continue;
          }
          if(!live_graph_.moves->Contain(getNode(u), getNode(d))){
            live_graph_.moves->Append(getNode(u), getNode(d));
          }
        }
      }
    }
    else{
      if(typeid(*(node->NodeInfo())) == typeid(assem::OperInstr)){
        printf("node OperInstr assem is %s\n", ((assem::OperInstr*)(node->NodeInfo()))->assem_.c_str());
      }
      // if(out_->Look(node)){
      //   if(!def){
      //     printf("!def\n");
      //     continue;
      //   }
        for(auto d :def->GetList()){
          auto temp_list = out_->Look(node)->GetList();
          for(auto temp : temp_list){
            if(d == reg_manager->StackPointer() || temp == reg_manager->StackPointer()){
              continue;
            }
            auto dst = getNode(d);
            auto out = getNode(temp);
            if(dst != out){
              live_graph_.interf_graph->AddEdge(dst, out);
              live_graph_.interf_graph->AddEdge(out, dst);
            }
          }
        }
      // }
    }
  }
  printf("end interf graph\n");
}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
