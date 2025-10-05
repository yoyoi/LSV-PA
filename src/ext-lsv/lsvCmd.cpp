#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include <set>
#include <map>
#include <vector>
#include <algorithm>


static int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv);
static int Lsv_CommandPrintMocut(Abc_Frame_t* pAbc, int argc, char** argv);


void init(Abc_Frame_t* pAbc) {
  Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
  Cmd_CommandAdd(pAbc, "LSV", "lsv_printmocut", Lsv_CommandPrintMocut, 0);
}

void destroy(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t frame_initializer = {init, destroy};

struct PackageRegistrationManager {
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer); }
} lsvPackageRegistrationManager;


void Lsv_NtkPrintNodes(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;
  Abc_NtkForEachNode(pNtk, pObj, i) {
    printf("Object Id = %d, name = %s\n", Abc_ObjId(pObj), Abc_ObjName(pObj));
    Abc_Obj_t* pFanin;
    int j;
    Abc_ObjForEachFanin(pObj, pFanin, j) {
      printf("  Fanin-%d: Id = %d, name = %s\n",
             j, Abc_ObjId(pFanin), Abc_ObjName(pFanin));
    }
    if (Abc_NtkHasSop(pNtk)) {
      printf("The SOP of this node:\n%s", (char*)pObj->pData);
    }
  }
}

int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }
  Lsv_NtkPrintNodes(pNtk);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_print_nodes [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}


void Lsv_EnumerateCuts(Abc_Obj_t* pNode, int k, std::set<std::set<int>>& cuts) {
  if (Abc_ObjIsPi(pNode)) {
    cuts.insert({Abc_ObjId(pNode)});
    return;
  }

  Abc_Obj_t* pF0 = Abc_ObjFanin0(pNode);
  Abc_Obj_t* pF1 = Abc_ObjFanin1(pNode);

  std::set<std::set<int>> cuts0, cuts1;
  Lsv_EnumerateCuts(pF0, k, cuts0);
  Lsv_EnumerateCuts(pF1, k, cuts1);

  for (auto& c0 : cuts0)
    for (auto& c1 : cuts1) {
      std::set<int> merged = c0;
      merged.insert(c1.begin(), c1.end());
      if ((int)merged.size() <= k)
        cuts.insert(merged);
    }


  cuts.insert({Abc_ObjId(pNode)});
}


int Lsv_CommandPrintMocut(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }

  if (!Abc_NtkIsStrash(pNtk)) {
    Abc_Print(-1, "Network must be an AIG. Run 'strash' first.\n");
    return 1;
  }

  if (argc < 3) {
    Abc_Print(-1, "usage: lsv_printmocut <k> <l>\n");
    return 1;
  }

  int k = atoi(argv[1]);
  int l = atoi(argv[2]);

  if (k < 3 || k > 6) {
    Abc_Print(-1, "k must be between 3 and 6.\n");
    return 1;
  }
  if (l < 1 || l > 4) {
    Abc_Print(-1, "l must be between 1 and 4.\n");
    return 1;
  }

  std::map<std::set<int>, std::set<int>> cut_to_outputs;

  Abc_Obj_t* pNode;
  int i;
  Abc_NtkForEachNode(pNtk, pNode, i) {
    if (!Abc_AigNodeIsAnd(pNode)) continue;

    std::set<std::set<int>> cuts;
    Lsv_EnumerateCuts(pNode, k, cuts);


    for (auto& cut : cuts)
      cut_to_outputs[cut].insert(Abc_ObjId(pNode));
  }

  for (auto& kv : cut_to_outputs) {
    const std::set<int>& inputs = kv.first;
    const std::set<int>& outputs = kv.second;
    if ((int)outputs.size() < l) continue;

    bool first = true;
    for (int in_id : inputs) {
      if (!first) printf(" ");
      printf("%d", in_id);
      first = false;
    }
    printf(" : ");
    first = true;
    for (int out_id : outputs) {
      if (!first) printf(" ");
      printf("%d", out_id);
      first = false;
    }
    printf("\n");
  }

  return 0;
}
