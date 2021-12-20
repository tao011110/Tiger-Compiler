#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result();
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
  private:
    frame::Frame *frame_;
    std::unique_ptr<cg::AssemInstr> assem_instr_;
    std::unique_ptr<ra::Result> result;
    live::LiveGraphFactory *liveGraph;
    int time = 0;

  public:
    RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr);
      
    void RegAlloc();
    std::unique_ptr<ra::Result> TransferResult();

  private:
    live::INodeListPtr precolored;
    live::INodeListPtr initial;

    live::INodeListPtr simplyWorklist;
    live::INodeListPtr freezeWorklist;
    live::INodeListPtr spillWorklist;
    live::INodeListPtr spilledNodes;
    live::INodeListPtr coalescedNodes;
    live::INodeListPtr coloredNodes;
    live::INodeListPtr selectStack;

    live::MoveList *coalescedMoves;
    live::MoveList *constrainedMoves;
    live::MoveList *frozenMoves;
    live::MoveList *worklistMoves;
    live::MoveList *activeMoves;

    std::set<std::pair<live::INodePtr, live::INodePtr>> adjSet;
    std::map<live::INodePtr, live::INodeListPtr> adjList;
    std::map<live::INodePtr, int> degree;
    std::map<live::INodePtr, live::MoveList *> moveList;
    std::map<live::INodePtr, live::INodePtr> alias;
    std::map<temp::Temp*, temp::Temp*> color;
    std::set<temp::Temp*> notSpill;

  public:
    live::INodeListPtr Union(live::INodeListPtr left, live::INodeListPtr right);
    live::INodeListPtr Sub(live::INodeListPtr left, live::INodeListPtr right);
    live::INodeListPtr Intersect(live::INodeListPtr left, live::INodeListPtr right);
    bool Contain(live::INodePtr node, live::INodeListPtr list);
    live::MoveList *Union(live::MoveList *left, live::MoveList *right);
    live::MoveList *Intersect(live::MoveList *left, live::MoveList *right);
    bool Contain(std::pair<live::INodePtr, live::INodePtr> node, live::MoveList *list);

    void Build();
    void AddEdge(live::INodePtr, live::INodePtr);
    void MakeWorkList();
    live::INodeListPtr Adjacent(live::INodePtr node);
    live::MoveList *NodeMoves(live::INodePtr node);
    bool MoveRelated(live::INodePtr node);
    void Simplify();
    void DecrementDegree(live::INodePtr node);
    void EnableMoves(live::INodeListPtr nodeList);
    void Coalesce();
    void addWorkList(live::INodePtr u);
    bool OK(live::INodePtr t, live::INodePtr r);
    bool Conservertive(live::INodeListPtr nodes);
    live::INodePtr GetAlias(live::INodePtr n);
    void Combine(live::INodePtr u, live::INodePtr v);
    void Freeze();
    void FreezeMoves(live::INodePtr u);
    void SelectSpill();
    void AssignColor();
    void RewriteProgram();
    void simplifyInstrList();
};

} // namespace ra

#endif