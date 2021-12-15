#include "tiger/regalloc/color.h"

extern frame::RegManager *reg_manager;

namespace col {
/* TODO: Put your lab6 code here */
    bool Color::checkPrecolored(live::INodePtr node){
        for(auto temp : precolored->GetList()){
            if(temp->NodeInfo() == node->NodeInfo()){
                return true;
            }
        }
        return false;
    }
    live::INodeListPtr Color::Union(live::INodeListPtr left, live::INodeListPtr right){
        live::INodeListPtr result = new live::INodeList();
        for(auto l : left->GetList()){
            if(!Contain(l, result)){
                result->Append(l);
            }
        }
        for(auto r : right->GetList()){
            if(!Contain(r, result)){
                result->Append(r);
            }
        }

        return result;
    }

    live::INodeListPtr Color::Sub(live::INodeListPtr left, live::INodeListPtr right){
        live::INodeListPtr result = new live::INodeList();
        for(auto l : left->GetList()){
            if(!Contain(l, right)){
                result->Append(l);
            }
        }

        return result;
    }

    live::INodeListPtr Color::Intersect(live::INodeListPtr left, live::INodeListPtr right){
        live::INodeListPtr result = new live::INodeList();
        for(auto l : left->GetList()){
            if(Contain(l, right)){
                result->Append(l);
            }
        }

        return result;
    }

    bool Color::Contain(live::INodePtr node, live::INodeListPtr list){
        for(auto tmp : list->GetList()){
            if(tmp->NodeInfo() == node->NodeInfo()){
                return true;
            }
        }

        return false;
    }

    live::MoveList *Union(live::MoveList *left, live::MoveList *right){
        live::MoveList *result = new live::MoveList();
        for(auto l : left->GetList()){
            if(!Contain(l, result)){
                result->Append(l.first, l.second);
            }
        }
        for(auto r : right->GetList()){
            if(!Contain(r, result)){
                result->Append(r.first, r.second);
            }
        }

        return result;
    }

    live::MoveList *Intersect(live::MoveList *left, live::MoveList *right){
        live::MoveList *result = new live::MoveList();
        for(auto l : left->GetList()){
            if(Contain(l, right)){
                result->Append(l.first, l.second);
            }
        }

        return result;
    }

    bool Contain(std::pair<live::INodePtr, live::INodePtr> node, live::MoveList *list){
        for(auto tmp : list->GetList()){
            if(tmp == node){
                return true;
            }
        }

        return false;
    }

    void Color::Build(){
        for(auto tmp : liveGraph.moves->GetList()){
            live::INodePtr src = tmp.first;
            live::INodePtr dst = tmp.second;
            live::MoveList *src_list = moveList[src];
            if(!src_list){
               src_list = new live::MoveList();
            }
            src_list->Append(src, dst);
            moveList[src] = src_list;
            live::MoveList *dst_list = moveList[dst];
            if(!dst_list){
               dst_list = new live::MoveList();
            }
            dst_list->Append(dst, dst);
            moveList[dst] = dst_list;
        }
        worklistMoves = liveGraph.moves;
        for(auto node : liveGraph.interf_graph->Nodes()->GetList()){
            for(auto adj : node->Adj()->GetList()){
                AddEdge(node, adj);
            }
        }

    }

    void Color::AddEdge(live::INodePtr u, live::INodePtr v){
        std::pair<live::INodePtr, live::INodePtr> edge = std::make_pair(u, v);
        if(adjSet.find(edge) == adjSet.end() && u != v){
            if(!checkPrecolored(u)){
                live::INodeListPtr list = adjList[u];
                list->Append(v);
                adjList[u] = list;
                degree[u]++;
            }
            if(!checkPrecolored(v)){
                live::INodeListPtr list = adjList[v];
                list->Append(u);
                adjList[v] = list;
                degree[v]++;
            }
        }
    }

    void Color::MakeWorkList(){
        int K = reg_manager->Registers()->GetList().size();
        for(auto node : initial->GetList()){
            if(degree[node] >= K){
                spillWorklist->Append(node);
            }
            else{
                if(MoveRelated(node)){
                    freezeWorklist->Append(node);
                }
                else{
                    simplyWorklist->Append(node);
                }
            }
        }
    }

    live::INodeListPtr Color::Adjacent(live::INodePtr node){
        return Sub(adjList[node], Union(selectStack, coalescedNodes));
    }

    live::MoveList *Color::NodeMoves(live::INodePtr node){
        return Intersect(moveList[node], Union(activeMoves, worklistMoves));
    }

    bool Color::MoveRelated(live::INodePtr node){
        if(NodeMoves(node)->GetList().size() == 0){
            return false;
        }
        return true;
    }

    void Color::Simplify(){
        if(simplyWorklist->GetList().empty()){
            return;
        }
        auto node = simplyWorklist->GetList().begin();
        simplyWorklist->DeleteNode(*node);
        selectStack->Append(*node);
        live::INodeListPtr adj = Adjacent(*node);
        for(auto tmp : adj->GetList()){
            DecrementDegree(tmp);
        }
    }

    void Color::DecrementDegree(live::INodePtr node){
        int d = degree[node];
        degree[node] = d - 1;
        if(d == (reg_manager->Registers()->GetList().size())){
            live::INodeListPtr list = Adjacent(node);
            list->Append(node);
            EnableMoves(list);
            spillWorklist->DeleteNode(node);
            if(MoveRelated(node)){
                freezeWorklist->Append(node);
            }
            else{
                simplyWorklist->Append(node);
            }
        }
    }

    void Color::EnableMoves(live::INodeListPtr nodeList){
        for(auto n : nodeList->GetList()){
            for(auto m : NodeMoves(n)->GetList()){
                if(Contain(m, activeMoves)){
                    activeMoves->Delete(m.first, m.second);
                    worklistMoves->Append(m.first, m.second);
                }
            }
        }
    }

    void Color::Coalesce(){
        if(worklistMoves->GetList().empty()){
            return;
        }
        auto m = worklistMoves->GetList().begin();
        live::INodePtr x = (*m).first;
        live::INodePtr y = (*m).second;
        x = GetAlias(x);
        y = GetAlias(y);
        live::INodePtr u = nullptr;
        live::INodePtr v = nullptr;
        if(Contain(y, precolored)){
            u = y;
            v = x;
        }
        else{
            u = x;
            v = y;
        }
        worklistMoves->Delete((*m).first, (*m).second);
        if(u == v){
            coalescedMoves->Append((*m).first, (*m).second);
            addWorkList(u);
        }
        else{
            if(Contain(v, precolored) || adjSet.find(std::make_pair(u, v)) != adjSet.end()){
                constrainedMoves->Append((*m).first, (*m).second);
                addWorkList(u);
                addWorkList(v);
            }
            else{
                bool flag = true;
                for(auto t : Adjacent(v)->GetList()){
                    if(!OK(t, u)){
                        flag = false;
                        break;
                    }
                }

                if(Contain(u, precolored) && flag 
                    || !Contain(u, precolored) && Conservertive(Union(Adjacent(v), Adjacent(u)))){
                        coalescedMoves->Append((*m).first, (*m).second);
                        Combine(u, v);
                        addWorkList(u);
                }
                else{
                    activeMoves->Append((*m).first, (*m).second);
                }
            }
        }
    }

    void Color::addWorkList(live::INodePtr u){
        if(!Contain(u, precolored) && !MoveRelated(u) && degree[u] < reg_manager->Registers()->GetList().size()){
            freezeWorklist->DeleteNode(u);
            simplyWorklist->Append(u);
        }
    }

    bool Color::OK(live::INodePtr t, live::INodePtr r){
        if(degree[t] < reg_manager->Registers()->GetList().size() || Contain(t, precolored)){
            return true;
        }
        else{
            if(adjSet.find(std::make_pair(t, r)) != adjSet.end()){
                return true;
            }
        }
        return false;
    }

    bool Color::Conservertive(live::INodeListPtr nodes){
        int k = 0;
        int K = reg_manager->Registers()->GetList().size();
        for(auto n : nodes->GetList()){
            if(degree[n] >= K){
                k++;
            }
        }
        return k < K;
    }

    live::INodePtr Color::GetAlias(live::INodePtr n){
        if(Contain(n, coalescedNodes)){
            return GetAlias(alias[n]);
        }
        else{
            return n;
        }
    }

    void Color::Combine(live::INodePtr u, live::INodePtr v){
        if(freezeWorklist->Contain(v)){
            freezeWorklist->DeleteNode(v);
        }
        else{
            spillWorklist->DeleteNode(v);
        }
        coloredNodes->Append(v);
        alias[v] = u;
        moveList[u]->Union(moveList[v]);
        live::INodeListPtr v_list = new live::INodeList();
        v_list->Append(v);
        EnableMoves(v_list);
        for(auto t : Adjacent(v)->GetList()){
            AddEdge(t, u);
            DecrementDegree(t);
        }
        if(degree[u] >= reg_manager->Registers()->GetList().size() && freezeWorklist->Contain(u)){
            freezeWorklist->DeleteNode(u);
            spillWorklist->Append(u);
        }
    }

} // namespace col
