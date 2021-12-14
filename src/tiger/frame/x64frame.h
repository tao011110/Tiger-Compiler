//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
private:
  temp::Temp *rax;
  temp::Temp *rbx;
  temp::Temp *rcx;
  temp::Temp *rdx;
  temp::Temp *rsi;
  temp::Temp *rdi;
  temp::Temp *rbp;
  temp::Temp *rsp;
  temp::Temp *r8;
  temp::Temp *r9;
  temp::Temp *r10;
  temp::Temp *r11;
  temp::Temp *r12;
  temp::Temp *r13;
  temp::Temp *r14;
  temp::Temp *r15;
  
public:
  X64RegManager(){
    rax = temp::TempFactory::NewTemp();
    rbx = temp::TempFactory::NewTemp();
    rcx = temp::TempFactory::NewTemp();
    rdx = temp::TempFactory::NewTemp();
    rsi = temp::TempFactory::NewTemp();
    rdi = temp::TempFactory::NewTemp();
    rbp = temp::TempFactory::NewTemp();
    rsp = temp::TempFactory::NewTemp();
    r8 = temp::TempFactory::NewTemp();
    r9 = temp::TempFactory::NewTemp();
    r10 = temp::TempFactory::NewTemp();
    r11 = temp::TempFactory::NewTemp();
    r12 = temp::TempFactory::NewTemp();
    r13 = temp::TempFactory::NewTemp();
    r14 = temp::TempFactory::NewTemp();
    r15 = temp::TempFactory::NewTemp();

    regs_=std::vector<temp::Temp*>{rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp, 
      r8, r9, r10, r11, r12, r13, r14, r15};

    temp::Map::Name()->Enter(rax,new std::string("%rax"));
    temp::Map::Name()->Enter(rbx,new std::string("%rbx"));
    temp::Map::Name()->Enter(rcx,new std::string("%rcx"));
    temp::Map::Name()->Enter(rdx,new std::string("%rdx"));
    temp::Map::Name()->Enter(rsi,new std::string("%rsi"));
    temp::Map::Name()->Enter(rdi,new std::string("%rdi"));
    temp::Map::Name()->Enter(rbp,new std::string("%rbp"));
    temp::Map::Name()->Enter(rsp,new std::string("%rsp"));
    temp::Map::Name()->Enter(r8,new std::string("%r8"));
    temp::Map::Name()->Enter(r9,new std::string("%r9"));
    temp::Map::Name()->Enter(r10,new std::string("%r10"));
    temp::Map::Name()->Enter(r11,new std::string("%r11"));
    temp::Map::Name()->Enter(r12,new std::string("%r12"));
    temp::Map::Name()->Enter(r13,new std::string("%r13"));
    temp::Map::Name()->Enter(r14,new std::string("%r14"));
    temp::Map::Name()->Enter(r15,new std::string("%r15"));
  }

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] temp::TempList *Registers(){
    temp::TempList *tempList = new temp::TempList({rax, rbx, rcx, rdx,
      rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15});

    return tempList;
  }

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] temp::TempList *ArgRegs(){
    temp::TempList *tempList = new temp::TempList({rdi, rsi, rdx, rcx, r8, r9});
    return tempList;
  }

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] temp::TempList *CallerSaves(){
    temp::TempList *tempList = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11});
    // these register set can't overlap each other?
    // temp::TempList *tempList = new temp::TempList({r10, r11});
    return tempList;
  }

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] temp::TempList *CalleeSaves(){
    temp::TempList *tempList = new temp::TempList({rbx, rbp, r12, r13, r14, r15});
    return tempList;
  }

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  //TODO: not finish in lab5-part1 here
  [[nodiscard]] temp::TempList *ReturnSink(){
    temp::TempList *tempList = CalleeSaves();
    tempList->Append(StackPointer());
    tempList->Append(ReturnValue());

    return tempList;
  }

  /**
   * Get word size
   */
  [[nodiscard]] int WordSize(){
    return 8;
  }

  [[nodiscard]] temp::Temp *FramePointer(){
    return rbp;
  }

  [[nodiscard]] temp::Temp *StackPointer(){
    return rsp;
  }

  [[nodiscard]] temp::Temp *ReturnValue(){
    return rax;
  }
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H