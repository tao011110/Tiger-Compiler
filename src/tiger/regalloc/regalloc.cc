#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */ 
    Result::~Result(){
        delete il_;
        delete coloring_;
    }

    RegAllocator::RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr)
      : frame_(frame), assem_instr_(std::move(assem_instr)) {
        // fg::FlowGraphFactory *flowGraph = new fg::FlowGraphFactory(assem_instr_.get()->GetInstrList());
        // flowGraph->AssemFlowGraph();
        // liveGraph = new live::LiveGraphFactory(flowGraph->GetFlowGraph());

        // init these
        printf("begin RegAllocator\n");
        precolored = new live::INodeList();
        initial = new live::INodeList();

        simplyWorklist = new live::INodeList();
        freezeWorklist = new live::INodeList();
        spillWorklist = new live::INodeList();
        spilledNodes = new live::INodeList();
        coalescedNodes = new live::INodeList();
        coloredNodes = new live::INodeList();
        selectStack = new live::INodeList();

        coalescedMoves = new live::MoveList();
        constrainedMoves = new live::MoveList();
        frozenMoves = new live::MoveList();
        worklistMoves = new live::MoveList();
        activeMoves = new live::MoveList();
    }

    std::unique_ptr<ra::Result> RegAllocator::TransferResult(){
        return std::move(result);
    }

    bool RegAllocator::Contain(live::INodePtr node, live::INodeListPtr list){
        for(auto tmp : list->GetList()){
            if(tmp->NodeInfo() == node->NodeInfo()){
                return true;
            }
        }

        return false;
    }

    live::INodeListPtr RegAllocator::Union(live::INodeListPtr left, live::INodeListPtr right){
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

    live::INodeListPtr RegAllocator::Sub(live::INodeListPtr left, live::INodeListPtr right){
        live::INodeListPtr result = new live::INodeList();
        for(auto l : left->GetList()){
            if(!Contain(l, right)){
                result->Append(l);
            }
        }

        return result;
    }

    live::INodeListPtr RegAllocator::Intersect(live::INodeListPtr left, live::INodeListPtr right){
        live::INodeListPtr result = new live::INodeList();
        for(auto l : left->GetList()){
            if(Contain(l, right)){
                result->Append(l);
            }
        }

        return result;
    }

    bool RegAllocator::Contain(std::pair<live::INodePtr, live::INodePtr> node, live::MoveList *list){
        for(auto tmp : list->GetList()){
            if(tmp == node){
                return true;
            }
        }

        return false;
    }

    live::MoveList *RegAllocator::Union(live::MoveList *left, live::MoveList *right){
        live::MoveList *result = new live::MoveList();
        printf("do union\n");
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

    live::MoveList *RegAllocator::Intersect(live::MoveList *left, live::MoveList *right){
        live::MoveList *result = new live::MoveList();
        for(auto l : left->GetList()){
            if(Contain(l, right)){
                result->Append(l.first, l.second);
            }
        }

        return result;
    }

    void RegAllocator::Build(){
        for(auto tmp : liveGraph->GetLiveGraph().moves->GetList()){
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
            dst_list->Append(src, dst);
            moveList[dst] = dst_list;
        }
        worklistMoves = liveGraph->GetLiveGraph().moves;
        for(auto node : liveGraph->GetLiveGraph().interf_graph->Nodes()->GetList()){
            for(auto adj : node->Adj()->GetList()){
                AddEdge(node, adj);
            }
        }

        // init precolored
        precolored->Clear();
        std::list<temp::Temp *> regs = reg_manager->Registers()->GetList();
        for(auto node : liveGraph->GetLiveGraph().interf_graph->Nodes()->GetList()){
            for(auto reg : regs){
                if(node->NodeInfo() == reg){
                    precolored->Append(node);
                    break;
                }
            }
        }
        printf("precolored size is %d\n", int(precolored->GetList().size()));

        // the initial here?
        for(auto node : liveGraph->GetLiveGraph().interf_graph->Nodes()->GetList()){
            bool flag = true;
            std::list<live::INodePtr> precolored_list = precolored->GetList();
            auto tmp = precolored_list.begin();
            for(; tmp != precolored_list.end(); tmp++){
                if(*tmp == node){
                    printf("*tmp == node with %d\n", (*tmp)->NodeInfo()->Int());
                    color[node->NodeInfo()] = (*tmp)->NodeInfo();
                    flag = false;
                    break;
                }
            }
            if(flag){
                printf("initial->Append(node) %d\n", node->NodeInfo()->Int());
                initial->Append(node);
            }
            alias[node] = node;
        }
    }

    void RegAllocator::AddEdge(live::INodePtr u, live::INodePtr v){
        std::pair<live::INodePtr, live::INodePtr> edge = std::make_pair(u, v);
        if(adjSet.find(edge) == adjSet.end() && u != v){
            adjSet.insert(std::make_pair(u, v));
            adjSet.insert(std::make_pair(v, u));
            if(!precolored->Contain(u)){
                adjList[u]->Append(v);
                if(!degree[u]){
                    degree[u] = 0;
                }
                degree[u]++;
            }

            if(!precolored->Contain(v)){
                adjList[v]->Append(u);
                if(!degree[v]){
                    degree[v] = 0;
                }
                degree[v]++;
            }
        }
    }

    void RegAllocator::MakeWorkList(){
        int K = reg_manager->Registers()->GetList().size();
        for(auto node : initial->GetList()){
            // printf("initial has %d\n", node->NodeInfo()->Int());
            if(degree[node] >= K){
                printf("spillWorklist->Append(node) %d\n", node->NodeInfo()->Int());
                spillWorklist->Append(node);
            }
            else{
                if(MoveRelated(node)){
                    printf("freezeWorklist->Append(node) %d\n", node->NodeInfo()->Int());
                    freezeWorklist->Append(node);
                }
                else{
                    printf("MakeWorkList\n");
                    printf("simplyWorklist->Append(node) %d\n", node->NodeInfo()->Int());
                    simplyWorklist->Append(node);
                }
            }
        }
        printf("spillWorklist.size is %d\n", int(spillWorklist->GetList().size()));
        initial->Clear();
    }

    live::INodeListPtr RegAllocator::Adjacent(live::INodePtr node){
        return adjList[node]->Diff(selectStack->Union(coalescedNodes));
    }

    live::MoveList *RegAllocator::NodeMoves(live::INodePtr node){
        if(!moveList[node]){
            // printf("%d is null\n", node->NodeInfo()->Int());
            return new live::MoveList();
        }
        return moveList[node]->Intersect(Union(activeMoves, worklistMoves));
    }

    bool RegAllocator::MoveRelated(live::INodePtr node){
        if(NodeMoves(node)->GetList().size() == 0){
            return false;
        }
        return true;
    }

    void RegAllocator::Simplify(){
        if(simplyWorklist->GetList().empty()){
            return;
        }
        auto node = simplyWorklist->GetList().begin();
        simplyWorklist->DeleteNode(*node);
        printf("simplify\n");
        printf("selectStack->Append(*node) %d\n", (*node)->NodeInfo()->Int());
        selectStack->Append(*node);
        live::INodeListPtr adj = Adjacent(*node);
        for(auto tmp : adj->GetList()){
            DecrementDegree(tmp);
        }
    }

    void RegAllocator::DecrementDegree(live::INodePtr node){
        int d = degree[node];
        degree[node] = d - 1;
        if(d == (reg_manager->Registers()->GetList().size())){
            live::INodeListPtr list = Adjacent(node);
            list->Append(node);
            EnableMoves(list);
            spillWorklist->DeleteNode(node);
            if(MoveRelated(node)){
                printf("freezeWorklist->Append(node) %d\n", node->NodeInfo()->Int());
                freezeWorklist->Append(node);
            }
            else{
                printf("DecrementDegree\n");
                printf("simplyWorklist->Append(node) %d\n", node->NodeInfo()->Int());
                simplyWorklist->Append(node);
            }
        }
    }

    void RegAllocator::EnableMoves(live::INodeListPtr nodeList){
        for(auto n : nodeList->GetList()){
            for(auto m : NodeMoves(n)->GetList()){
                if(activeMoves->Contain(m.first, m.second)){
                    activeMoves->Delete(m.first, m.second);
                    worklistMoves->Append(m.first, m.second);
                    // printf("EnableMoves \n");
                    // printf("m.first is %d", m.first->NodeInfo()->Int());
                }
            }
        }
    }

    void RegAllocator::Coalesce(){
        if(worklistMoves->GetList().empty()){
            return;
        }
        // printf("Coalesce\n");
        auto m = worklistMoves->GetList().begin();
        live::INodePtr x = (*m).first;
        // printf("x is %d\n", x->NodeInfo()->Int());
        live::INodePtr y = (*m).second;
        // printf("y is %d\n", y->NodeInfo()->Int());
        x = GetAlias(x);
        y = GetAlias(y);
        live::INodePtr u = nullptr;
        live::INodePtr v = nullptr;
        if(precolored->Contain(y)){
            u = y;
            v = x;
        }
        else{
            u = x;
            v = y;
        }
        worklistMoves->Delete((*m).first, (*m).second);
        if(u == v){
            if(!coalescedMoves->Contain((*m).first, (*m).second)){
                coalescedMoves->Append((*m).first, (*m).second);
            }
            addWorkList(u);
        }
        else{
            if(precolored->Contain(v) || adjSet.find(std::make_pair(u, v)) != adjSet.end()){
                if(!constrainedMoves->Contain((*m).first, (*m).second)){
                   constrainedMoves->Append((*m).first, (*m).second);
                }
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

                if(precolored->Contain(u) && flag 
                    || !precolored->Contain(u) && Conservertive(Adjacent(u)->Union(Adjacent(v)))){
                        if(!coalescedMoves->Contain((*m).first, (*m).second)){
                            coalescedMoves->Append((*m).first, (*m).second);
                        }
                        Combine(u, v);
                        addWorkList(u);
                }
                else{
                    if(!activeMoves->Contain((*m).first, (*m).second)){
                        activeMoves->Append((*m).first, (*m).second);
                    }
                }
            }
        }
    }

    void RegAllocator::addWorkList(live::INodePtr u){
        if(!precolored->Contain(u) && !MoveRelated(u) && degree[u] < reg_manager->Registers()->GetList().size()){
            freezeWorklist->DeleteNode(u);
            printf("addWorkList\n");
            printf("simplyWorklist->Append(node) %d\n", u->NodeInfo()->Int());
            simplyWorklist->Append(u);
        }
    }

    bool RegAllocator::OK(live::INodePtr t, live::INodePtr r){
        if(degree[t] < reg_manager->Registers()->GetList().size() || precolored->Contain(t)){
            return true;
        }
        else{
            if(adjSet.find(std::make_pair(t, r)) != adjSet.end()){
                return true;
            }
        }
        return false;
    }

    bool RegAllocator::Conservertive(live::INodeListPtr nodes){
        int k = 0;
        int K = reg_manager->Registers()->GetList().size();
        for(auto n : nodes->GetList()){
            if(degree[n] >= K){
                k++;
            }
        }
        return k < K;
    }

    live::INodePtr RegAllocator::GetAlias(live::INodePtr n){
        if(coalescedNodes->Contain(n)){
            return GetAlias(alias[n]);
        }
        else{
            return n;
        }
    }

    void RegAllocator::Combine(live::INodePtr u, live::INodePtr v){
        if(freezeWorklist->Contain(v)){
            freezeWorklist->DeleteNode(v);
            printf("at combine freezeWorklist->DeleteNode(v) %d\n", v->NodeInfo()->Int());
        }
        else{
            spillWorklist->DeleteNode(v);
            printf("at combine spillWorklist->DeleteNode(v) %d\n", v->NodeInfo()->Int());
        }
        coalescedNodes->Append(v);
        alias[v] = u;
        moveList[u] = moveList[u]->Union(moveList[v]);
        live::INodeListPtr v_list = new live::INodeList();
        v_list->Append(v);
        EnableMoves(v_list);
        for(auto t : Adjacent(v)->GetList()){
            AddEdge(t, u);
            AddEdge(u, t);
            DecrementDegree(t);
        }
        if(degree[u] >= reg_manager->Registers()->GetList().size() && freezeWorklist->Contain(u)){
            freezeWorklist->DeleteNode(u);
            spillWorklist->Append(u);
            printf("at combine spillWorklist->Append(u) %d\n", u->NodeInfo()->Int());
        }
    }

    void RegAllocator::Freeze(){
        if(freezeWorklist->GetList().empty()){
            return;
        }
        auto u = freezeWorklist->GetList().begin();
        freezeWorklist->DeleteNode(*u);
        printf("Freeze\n");
        printf("simplyWorklist->Append(node) %d\n", (*u)->NodeInfo()->Int());
        simplyWorklist->Append(*u);
        FreezeMoves(*u);
    }

    void RegAllocator::FreezeMoves(live::INodePtr u){
        for(auto m : NodeMoves(u)->GetList()){
            live::INodePtr x = m.first;
            live::INodePtr y = m.second;
            live::INodePtr v = nullptr;
            if(GetAlias(y) == GetAlias(u)){
                v = GetAlias(x);
            }
            else{
                v = GetAlias(y);
            }
            activeMoves->Delete(m.first, m.second);
            if(!frozenMoves->Contain(m.first, m.second)){
                frozenMoves->Append(m.first, m.second);
            }
            if(NodeMoves(v)->GetList().empty() && degree[v] < reg_manager->Registers()->GetList().size()){
                freezeWorklist->DeleteNode(v);
                printf("FreezeMoves\n");
                printf("simplyWorklist->Append(node) %d\n", v->NodeInfo()->Int());
                simplyWorklist->Append(v);
            }
        }
    }

    void RegAllocator::SelectSpill(){
        // select the node with highest degree
        live::INodePtr m = *(spillWorklist->GetList().begin());
        int max_degree = degree[m];
        for(auto tmp : spillWorklist->GetList()){
            // don't spill new temp reg
            if(notSpill.find(tmp->NodeInfo()) != notSpill.end()){
                continue;
            }
            if(!spilledNodes->Contain(tmp) && !precolored->Contain(tmp)){
                int d = degree.find(tmp)->second;
                if(d >= max_degree){
                    m = tmp;
                    max_degree = d;
                }
            }
        }
        printf("spillWorklist delete %d\n", m->NodeInfo()->Int());
        spillWorklist->DeleteNode(m);
        simplyWorklist->Append(m);
        FreezeMoves(m);
    }

    void RegAllocator::AssignColor(){
        spilledNodes->Clear();
        std::list<live::INodePtr> list = selectStack->GetList();
        printf("selectStack size is %d\n", (int)selectStack->GetList().size());
        auto n = list.end();
        while(n != list.begin()){
            n--;
            printf("shssh\n");
            std::vector<temp::Temp*> okColors;
            int K = reg_manager->Registers()->GetList().size();
            for(int i = 0; i < K; i++){
                okColors.push_back(reg_manager->Registers()->NthTemp(i));
            }

            for(auto w : adjList[*n]->GetList()){
                if((coloredNodes->Union(precolored))->Contain(GetAlias(w))){
                    // printf("we want to get %d\n", GetAlias(w)->NodeInfo()->Int());
                    auto t = color.begin();
                    for(; t != color.end(); t++){
                        if((*t).first == GetAlias(w)->NodeInfo()){
                            break;
                        }
                    }
                    if(t == color.end()){
                        continue;
                    }
                    for(auto del = okColors.begin(); del != okColors.end(); del++){
                        if(*del == (*t).second){
                            // printf("del!!! %d\n", (*t).second->Int());
                            okColors.erase(del);
                            break;
                        }
                    }
                }
            }
            if(okColors.empty()){
                printf("spilledNodes->Append(*n) \n");
                spilledNodes->Append(*n);
            }
            else{
                coloredNodes->Append(*n);
                // be careful with the back!!!!
                auto c = okColors.back();
                color[(*n)->NodeInfo()] = c;
                printf("first is %d\n", (*n)->NodeInfo()->Int());
                printf("second is %d\n", c->Int());
                // printf("color size is %d\n", (int)color.size());
                // printf("\n");
                // for(auto tmp : color){
                //     printf("tmp first %d\n", tmp.first->Int());
                // }
                // printf("\n");
            }
        }
        selectStack->Clear();

        for(auto n : coalescedNodes->GetList()){
            printf("coalescedNode %d  and  %d\n", n->NodeInfo()->Int(), GetAlias(n)->NodeInfo()->Int());
            color[n->NodeInfo()] = color[GetAlias(n)->NodeInfo()];
        }
    }

    void RegAllocator::RewriteProgram(){
        printf("do RewriteProgram\n");
        assem::InstrList *new_instrList = new assem::InstrList();
        std::vector<int> offset_vec;
        for(auto node : spilledNodes->GetList()){
            frame_->allocLocal(true);
            offset_vec.push_back(frame_->offset);
        }
        assem::InstrList *instr_list = assem_instr_.get()->GetInstrList();
        for(auto il = instr_list->GetList().begin(); il != instr_list->GetList().end(); il++){
            int i = 0;
            assem::InstrList *temp_instrList = new assem::InstrList();
            std::vector<temp::Temp *> node_temp_vec;
            for(auto node : spilledNodes->GetList()){
                temp::TempList *use = (*il)->Use();
                if(live::Contain(node->NodeInfo(), use)){
                    temp::Temp *t = temp::TempFactory::NewTemp();
                    node_temp_vec.push_back(t);
                    notSpill.insert(t);
                    std::string instr = std::string("movq (") + frame_->name->Name() + std::string("_framesize-") + 
                        std::to_string(-(offset_vec[i])) + std::string(")(`s0), `d0");
                    // insert before il
                    new_instrList->Append(new assem::OperInstr(instr, new temp::TempList(t), 
                    new temp::TempList(reg_manager->StackPointer()), nullptr));
                    printf("add in %s\n", instr.c_str());

                    if(typeid(*(*il)) == typeid(assem::MoveInstr)){
                        printf("the dup assem is %s\n", ((assem::MoveInstr*)(*il))->assem_.c_str());
                        temp::TempList *src_list = ((assem::MoveInstr*)(*il))->src_;
                        temp::TempList *src_list_new = new temp::TempList();
                        for(auto src : src_list->GetList()){
                            if(src->Int() == node->NodeInfo()->Int()){
                                src_list_new->Append(t);
                                continue;
                            }
                            src_list_new->Append(src);
                        }
                        ((assem::MoveInstr*)(*il))->src_ = src_list_new;
                    }
                    else{
                        if(typeid(*(*il)) == typeid(assem::OperInstr)){
                            printf("the dup assem is %s\n", ((assem::OperInstr*)(*il))->assem_.c_str());
                            temp::TempList *src_list = ((assem::OperInstr*)(*il))->src_;
                            temp::TempList *src_list_new = new temp::TempList();
                            for(auto src : src_list->GetList()){
                                if(src->Int() == node->NodeInfo()->Int()){
                                    src_list_new->Append(t);
                                    continue;
                                }
                                src_list_new->Append(src);
                            }
                            ((assem::OperInstr*)(*il))->src_ = src_list_new;
                        }
                    }
                    printf("the spill is %d\n", node->NodeInfo()->Int());
                    printf("the new t is %d\n", t->Int());
                }
                else{
                    node_temp_vec.push_back(nullptr);
                }
                i++;
            }

            i = 0;
            for(auto node : spilledNodes->GetList()){
                temp::TempList *def = (*il)->Def();
                if(live::Contain(node->NodeInfo(), def)){
                    temp::Temp *t = node_temp_vec[i];
                    if(!t){
                        t = temp::TempFactory::NewTemp();
                        notSpill.insert(t);
                    }
                    std::string instr = std::string("movq `s0, (") + frame_->name->Name() + std::string("_framesize-") + 
                        std::to_string(-(offset_vec[i])) + std::string(")(`d0)");
                    temp_instrList->Append(new assem::OperInstr(instr, new temp::TempList(reg_manager->StackPointer()),
                        new temp::TempList(t), nullptr));
                    printf("add in %s\n", instr.c_str());

                    if(typeid(*(*il)) == typeid(assem::MoveInstr)){
                        temp::TempList *dst_list = ((assem::MoveInstr*)(*il))->dst_;
                        temp::TempList *dst_list_new = new temp::TempList();
                        for(auto dst : dst_list->GetList()){
                            if(dst->Int() == node->NodeInfo()->Int()){
                                dst_list_new->Append(t);
                                continue;
                            }
                            dst_list_new->Append(dst);
                        }
                        ((assem::MoveInstr*)(*il))->dst_ = dst_list_new;
                    }
                    else{
                        if(typeid(*(*il)) == typeid(assem::OperInstr)){
                            temp::TempList *dst_list = ((assem::OperInstr*)(*il))->dst_;
                            temp::TempList *dst_list_new = new temp::TempList();
                            for(auto dst : dst_list->GetList()){
                                if(dst->Int() == node->NodeInfo()->Int()){
                                    dst_list_new->Append(t);
                                    continue;
                                }
                                dst_list_new->Append(dst);
                            }
                            ((assem::OperInstr*)(*il))->dst_ = dst_list_new;
                        }
                    }
                }
                i++;
            }

            // insert il
            new_instrList->Append(*il);

            // insert after il
            for(auto instr : temp_instrList->GetList()){
                new_instrList->Append(instr);
            }
        }
        
        assem_instr_ = std::make_unique<cg::AssemInstr>(new_instrList);
        // spilledNodes->Clear();
        // // and initial??
        // coloredNodes->Clear();
        // coalescedNodes->Clear();
    }

    void RegAllocator::RegAlloc(){
        time++;
        printf("do RegAlloc\n");
        fg::FlowGraphFactory *flowGraph = new fg::FlowGraphFactory(assem_instr_.get()->GetInstrList());
        flowGraph->AssemFlowGraph();
        liveGraph = new live::LiveGraphFactory(flowGraph->GetFlowGraph());
        liveGraph->Liveness();
        for(auto node : liveGraph->GetLiveGraph().interf_graph->Nodes()->GetList()){
            adjList[node] = new live::INodeList();
            degree[node] = 0;
            printf("the node is %d\n", node->NodeInfo()->Int());
        }
        printf("begin size is %d\n", (int)liveGraph->GetLiveGraph().interf_graph->Nodes()->GetList().size());
        Build();
        MakeWorkList();
        
        do{
            if(!(simplyWorklist->GetList().empty())){
                printf("do Simplify\n");
                Simplify();
            }
            if(!worklistMoves->GetList().empty()){
                printf("do Coalesce\n");
                Coalesce();
            }
            if(!freezeWorklist->GetList().empty()){
                printf("do Freeze\n");
                Freeze();
            }
            if(!spillWorklist->GetList().empty()){
                printf("do SelectSpill\n");
                printf("spillWorklist size is %d \n", (int)(spillWorklist->GetList().size()));
                SelectSpill();
            }
        } while(!simplyWorklist->GetList().empty() || !worklistMoves->GetList().empty()
            ||!freezeWorklist->GetList().empty() || !spilledNodes->GetList().empty());

        AssignColor();
        if(!spilledNodes->GetList().empty() && time < 8){
            RewriteProgram();
            
            precolored->Clear();
            initial->Clear();

            simplyWorklist->Clear();
            freezeWorklist->Clear();
            spillWorklist->Clear();
            spilledNodes->Clear();
            coalescedNodes->Clear();
            coloredNodes->Clear();
            selectStack->Clear();

            coalescedMoves = new live::MoveList();
            constrainedMoves = new live::MoveList();
            frozenMoves = new live::MoveList();
            worklistMoves = new live::MoveList();
            activeMoves = new live::MoveList();

            adjSet.clear();
            adjList.clear();
            degree.clear();
            moveList.clear();
            alias.clear();
            color.clear();

            RegAlloc();
        }
        else{
            temp::Map *map = temp::Map::Empty();
            // with wrong!
            for(auto tmp : color){
                printf("tmp first %d\n", tmp.first->Int());
                std::string *name = temp::Map::Name()->Look(tmp.second);
                printf("tmp second %s\n", name->c_str());
                map->Enter(tmp.first, name);
            }
            // for(auto reg : reg_manager->Registers()->GetList()){
            //     map->Enter(reg, temp::Map::Name()->Look(reg));
            //     printf("first %d and second %s\n", reg->Int(), temp::Map::Name()->Look(reg)->c_str());
            // }
            map->Enter(reg_manager->StackPointer(), new std::string("%rsp"));
            for(auto tmp : color){
                printf("first %d and second %d\n", tmp.first->Int(), tmp.second->Int());
            }
            printf("make result\n"); 
            for(auto il : assem_instr_.get()->GetInstrList()->GetList()){
                if(typeid(*il) == typeid(assem::OperInstr)){
                    printf("%s\n", ((assem::OperInstr*)(il))->assem_.c_str());
                }
            }
            simplifyInstrList();
            result = std::make_unique<ra::Result>(map, assem_instr_.get()->GetInstrList());
        }
    }

    void RegAllocator::simplifyInstrList(){
        assem::InstrList *instr_list = assem_instr_.get()->GetInstrList();
        assem::InstrList *instr_list_new = new assem::InstrList();
        for(auto il : instr_list->GetList()){
            if(typeid(*il) == typeid(assem::MoveInstr)){
                if(((assem::MoveInstr*)il)->dst_ == ((assem::MoveInstr*)il)->src_){
                    continue;
                }
            }
            instr_list_new->Append(il);
        }
        assem_instr_ = std::make_unique<cg::AssemInstr>(instr_list_new);
    }

} // namespace ra