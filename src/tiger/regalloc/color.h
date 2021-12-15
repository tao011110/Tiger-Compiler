#ifndef TIGER_COMPILER_COLOR_H
#define TIGER_COMPILER_COLOR_H

#include "tiger/codegen/assem.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/util/graph.h"
#include <map>
#include <set>

namespace col {
struct Result {
  Result() : coloring(nullptr), spills(nullptr) {}
  Result(temp::Map *coloring, live::INodeListPtr spills)
      : coloring(coloring), spills(spills) {}
  temp::Map *coloring;
  live::INodeListPtr spills;
};

class Color {
  /* TODO: Put your lab6 code here */
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
    std::map<fg::FGraphPtr, temp::Temp*> color;

    live::LiveGraph liveGraph;

  public:
    bool checkPrecolored(live::INodePtr node);
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
};
} // namespace col

#endif // TIGER_COMPILER_COLOR_H
