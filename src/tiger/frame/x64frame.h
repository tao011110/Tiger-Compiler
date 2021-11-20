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
  // temp::Temp *rax = temp::TempFactory::NewTemp();
  // temp::Temp *rbx = temp::TempFactory::NewTemp();
  // temp::Temp *rcx = temp::TempFactory::NewTemp();
  // temp::Temp *rdx = temp::TempFactory::NewTemp();
  // temp::Temp *rsi = temp::TempFactory::NewTemp();
  // temp::Temp *rdi = temp::TempFactory::NewTemp();
  // temp::Temp *rbp = temp::TempFactory::NewTemp();
  // temp::Temp *rsp = temp::TempFactory::NewTemp();
  // temp::Temp *r8 = temp::TempFactory::NewTemp();
  // temp::Temp *r9 = temp::TempFactory::NewTemp();
  // temp::Temp *r10 = temp::TempFactory::NewTemp();
  // temp::Temp *r11 = temp::TempFactory::NewTemp();
  // temp::Temp *r12 = temp::TempFactory::NewTemp();
  // temp::Temp *r13 = temp::TempFactory::NewTemp();
  // temp::Temp *r14 = temp::TempFactory::NewTemp();
  // temp::Temp *r15 = temp::TempFactory::NewTemp();
  temp::Temp *rax = nullptr;
  temp::Temp *rbx = nullptr;
  temp::Temp *rcx = nullptr;
  temp::Temp *rdx = nullptr;
  temp::Temp *rsi = nullptr;
  temp::Temp *rdi = nullptr;
  temp::Temp *rbp = nullptr;
  temp::Temp *rsp = nullptr;
  temp::Temp *r8 = nullptr;
  temp::Temp *r9 = nullptr;
  temp::Temp *r10 = nullptr;
  temp::Temp *r11 = nullptr;
  temp::Temp *r12 = nullptr;
  temp::Temp *r13 = nullptr;
  temp::Temp *r14 = nullptr;
  temp::Temp *r15 = nullptr;
  
public:
  X64RegManager(){
    temp_map_->Empty();
    temp_map_->Enter(rax, new std::string("%rax"));
    temp_map_->Enter(rax, new std::string("%rbx"));
    temp_map_->Enter(rax, new std::string("%rcx"));
    temp_map_->Enter(rax, new std::string("%rdx"));
    temp_map_->Enter(rax, new std::string("%rsi"));
    temp_map_->Enter(rax, new std::string("%rdi"));
    temp_map_->Enter(rax, new std::string("%rbp"));
    temp_map_->Enter(rax, new std::string("%rsp"));
    temp_map_->Enter(rax, new std::string("%r8"));
    temp_map_->Enter(rax, new std::string("%r9"));
    temp_map_->Enter(rax, new std::string("%r10"));
    temp_map_->Enter(rax, new std::string("%r11"));
    temp_map_->Enter(rax, new std::string("%r12"));
    temp_map_->Enter(rax, new std::string("%r13"));
    temp_map_->Enter(rax, new std::string("%r14"));
    temp_map_->Enter(rax, new std::string("%r15"));
  }

  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  temp::TempList *Registers(){
    temp::TempList *tempList = new temp::TempList({rax, rbx, rcx, rdx,
      rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15});

    return tempList;
  }

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  temp::TempList *ArgRegs(){
    temp::TempList *tempList = new temp::TempList({rdi, rsi, rdx, rcx, r8, r9});
    return tempList;
  }

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  temp::TempList *CallerSaves(){
    temp::TempList *tempList = new temp::TempList({rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11});
    return tempList;
  }

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  temp::TempList *CalleeSaves(){
    temp::TempList *tempList = new temp::TempList({rbx, rbp, r12, r13, r14, r15});
    return tempList;
  }

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  //TODO: not finish in lab5-part1 here
  temp::TempList *ReturnSink(){
    temp::TempList *tempList = new temp::TempList();
    return tempList;
  }

  /**
   * Get word size
   */
  int WordSize(){
    return 8;
  }

  temp::Temp *FramePointer(){
    return rbp;
  }

  temp::Temp *StackPointer(){
    return rsp;
  }

  temp::Temp *ReturnValue(){
    return rax;
  }

  temp::Map *temp_map_;

  std::vector<temp::Temp *> regs_ = {rax, rbx, rcx, rdx,
      rdi, rbp, rsp, r8, r9, r10, r11, r12, r13, r14, r15};
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
