#ifndef HEX_SIM_HPP
#define HEX_SIM_HPP

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <vector>

#include "hex.hpp"
#include "hexcontainer.hpp"
#include "heximage.hpp"
#include "hexsimio.hpp"

namespace hexsim {

/// Outcome of executing a single instruction.
enum class StepResult { RUNNING, HALTED, BLOCKED };

class Processor;

/// A point-to-point synchronous channel connecting two processors. Holds the
/// rendezvous state and (when a writer is parked) the value in flight.
struct Channel {
  enum class State { IDLE, WRITER_WAITING, READER_WAITING };
  State state = State::IDLE;
  uint32_t value = 0;
  Processor *writer = nullptr;
  Processor *reader = nullptr;
};

class Processor {

  // Constants.
  static const size_t MEMORY_SIZE_WORDS = hex::MAX_MEMORY_SIZE_WORDS;

  // State.
  uint32_t pc;
  uint32_t areg;
  uint32_t breg;
  uint32_t oreg;
  uint32_t instr;

  // Memory.
  std::array<uint32_t, MEMORY_SIZE_WORDS> memory;

  // IO.
  hex::HexSimIO io;
  // Control whether characters are sign extended into 32 bits. The behaviour of
  // xhexb.x is for character values to be truncated on conversion to 32 bits.
  // However, it is useful for testing to allow negative values.
  bool truncateInputs;

  // Logging.
  std::ostream &out;

  // Control.
  bool running;
  bool tracing;
  int exitCode;

  // Multi-processor network state.
  unsigned id = 0;
  StepResult status = StepResult::RUNNING;
  std::array<Channel *, hex::NUM_LINKS> links{}; // slot -> channel (null if
                                                 // unwired)
  unsigned blockedSlot = 0; // slot this processor is blocked on

  // State for tracing.
  uint32_t lastPC;
  size_t cycles;
  size_t maxCycles;
  hex::Instr instrEnum;
  std::vector<std::pair<std::string, unsigned>> debugInfo;
  std::map<std::string, unsigned> debugInfoMap;

  /// Lookup a symbol name given the current PC: the symbol with the greatest
  /// offset <= lastPC. debugInfo is sorted by ascending offset.
  const char *lookupSymbol() {
    auto it = std::upper_bound(
        debugInfo.begin(), debugInfo.end(), lastPC,
        [](uint32_t pc, const std::pair<std::string, unsigned> &entry) {
          return pc < entry.second;
        });
    if (it == debugInfo.begin()) {
      return nullptr; // lastPC is before the first symbol.
    }
    return std::prev(it)->first.c_str();
  }

public:
  Processor(std::istream &in, std::ostream &out, size_t maxCycles = 0)
      : pc(0), areg(0), breg(0), oreg(0), io(in, out), truncateInputs(true),
        out(out), running(true), tracing(false), lastPC(0), cycles(0),
        maxCycles(maxCycles) {}

  void setTracing(bool value) { tracing = value; }
  void setTruncateInputs(bool value) { truncateInputs = value; }

  void setId(unsigned value) { id = value; }
  void setLink(unsigned slot, Channel *channel) { links[slot] = channel; }
  StepResult getStatus() const { return status; }
  int getExitCode() const { return exitCode; }
  unsigned getBlockedSlot() const { return blockedSlot; }

  /// Load a single image (size-word + code + optional debug info) from a
  /// stream. imageSizeBytes is the total number of bytes the image occupies.
  /// Returns the program size in bytes.
  unsigned loadFromStream(std::istream &file, unsigned imageSizeBytes) {
    // Check the image length matches.
    unsigned remainingFileSize = imageSizeBytes - 4;
    remainingFileSize =
        (remainingFileSize + 3U) & ~3U; // Round up to multiple of 4.
    unsigned programSize = heximage::readU32(file) << 2;

    // Read the instructions into memory.
    file.read(reinterpret_cast<char *>(memory.data()), programSize);

    // Read debug data (if present).
    if (remainingFileSize > programSize) {
      for (const auto &symbol : heximage::readSymbols(file)) {
        debugInfo.push_back(std::make_pair(symbol.name, symbol.offset));
        debugInfoMap[symbol.name] = symbol.offset;
      }
    }

    return programSize;
  }

  /// Load a binary file as this processor's single image.
  void load(const char *filename, bool dumpContents = false) {
    std::ifstream file(filename, std::ios::binary);
    file.seekg(0, std::ios::end);
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    unsigned programSize =
        loadFromStream(file, static_cast<unsigned>(fileSize));

    // Print the contents of the binary.
    if (dumpContents) {
      out << "Read " << std::to_string(programSize) << " bytes\n";
      for (size_t i = 0; i < (programSize / 4) + 1; i++) {
        out << fmt::format("{:08d} {:08x}\n", i, memory[i]);
      }
    }
  }

  void traceSyscall() {
    unsigned spWordIndex = memory[1];
    switch (static_cast<hex::Syscall>(areg)) {
    case hex::Syscall::EXIT:
      out << fmt::format("exit {}\n", memory[spWordIndex + 2]);
      break;
    case hex::Syscall::WRITE:
      out << fmt::format("write {} to simout({})\n", memory[spWordIndex + 2],
                         memory[spWordIndex + 3]);
      break;
    case hex::Syscall::READ:
      out << fmt::format("read {} to mem[{:08x}]\n", memory[spWordIndex + 1],
                         (spWordIndex + 1));
      break;
    default:
      break;
    }
  }

  void trace(uint32_t instr, hex::Instr instrEnum) {
    if (debugInfo.size()) {
      auto symbolName = lookupSymbol();
      std::string symbolInfo;
      if (symbolName) {
        auto symbolOffset = lastPC - debugInfoMap[symbolName];
        symbolInfo = fmt::format("{}+{}", symbolName, symbolOffset);
      }
      out << fmt::format("{:<6d} {:<6d} {:<12} {:<4} {:<2d} ", cycles, lastPC,
                         symbolInfo, instrEnumToStr(instrEnum), (instr & 0xF));
    } else {
      out << fmt::format("{:<6d} {:<6d} {:<4} {:<2d} ", cycles, lastPC,
                         instrEnumToStr(instrEnum), (instr & 0xF));
    }
    switch (instrEnum) {
    case hex::Instr::LDAM:
      out << fmt::format("areg = mem[oreg ({:#08x})] ({})\n", oreg,
                         memory[oreg]);
      break;
    case hex::Instr::LDBM:
      out << fmt::format("breg = mem[oreg ({:#08x})] ({})\n", oreg,
                         memory[oreg]);
      break;
    case hex::Instr::STAM:
      out << fmt::format("mem[oreg ({:#08x})] = areg {}\n", oreg, areg);
      break;
    case hex::Instr::LDAC:
      out << fmt::format("areg = oreg {}\n", oreg);
      break;
    case hex::Instr::LDBC:
      out << fmt::format("breg = oreg {}\n", oreg);
      break;
    case hex::Instr::LDAP:
      out << fmt::format("areg = pc ({}) + oreg ({}) {}\n", pc, oreg,
                         (pc + oreg));
      break;
    case hex::Instr::LDAI:
      out << fmt::format("areg = mem[areg ({}) + oreg ({}) = {:#08x}] ({})\n",
                         areg, oreg, (areg + oreg), memory[areg + oreg]);
      break;
    case hex::Instr::LDBI:
      out << fmt::format("breg = mem[breg ({}) + oreg ({}) = {:#08x}] ({})\n",
                         breg, oreg, (breg + oreg), memory[breg + oreg]);
      break;
    case hex::Instr::STAI:
      out << fmt::format("mem[breg ({}) + oreg ({}) = {:#08x}] = areg ({})\n",
                         breg, oreg, (breg + oreg), areg);
      break;
    case hex::Instr::BR:
      out << fmt::format("pc = pc + oreg ({}) ({:#08x})\n", oreg, (pc + oreg));
      break;
    case hex::Instr::BRZ:
      out << fmt::format("pc = areg == zero ? pc + oreg ({}) ({:#08x}) : pc\n",
                         oreg, (pc + oreg));
      break;
    case hex::Instr::BRN:
      out << fmt::format("pc = areg < zero ? pc + oreg ({}) ({:#08x}) : pc\n",
                         oreg, (pc + oreg));
      break;
    case hex::Instr::PFIX:
      out << fmt::format("oreg = oreg ({}) << 4 ({:#08x})\n", oreg,
                         (oreg << 4));
      break;
    case hex::Instr::NFIX:
      out << fmt::format("oreg = 0xFFFFFF00 | oreg ({}) << 4 ({:#08x})\n", oreg,
                         (0xFFFFFF00 | (oreg << 4)));
      break;
    case hex::Instr::OPR:
      switch (static_cast<hex::OprInstr>(oreg)) {
      case hex::OprInstr::BRB:
        out << fmt::format("BRB pc = breg ({:#08x})\n", breg);
        break;
      case hex::OprInstr::ADD:
        out << fmt::format("ADD areg = areg ({}) + breg ({}) ({})\n", areg,
                           breg, (areg + breg));
        break;
      case hex::OprInstr::SUB:
        out << fmt::format("SUB areg = areg ({}) - breg ({}) ({})\n", areg,
                           breg, (areg - breg));
        break;
      case hex::OprInstr::SVC:
        break;
      };
      break;
    }
  }

  void syscall() {
    unsigned spWordIndex = memory[1];
    switch (static_cast<hex::Syscall>(areg)) {
    case hex::Syscall::EXIT:
      exitCode = memory[spWordIndex + 2];
      running = false;
      break;
    case hex::Syscall::WRITE:
      io.output(memory[spWordIndex + 2], memory[spWordIndex + 3]);
      break;
    case hex::Syscall::READ: {
      auto value = io.input(memory[spWordIndex + 2]);
      memory[spWordIndex + 1] = truncateInputs ? value & 0xFF : value;
      break;
    }
    default:
      throw std::runtime_error("invalid syscall: " + std::to_string(areg));
    }
  }

  /// Advance past the instruction just executed (commit PC, clear oreg).
  void advanceInstr() {
    lastPC = pc;
    pc = pc + 1;
    oreg = 0;
    cycles++;
  }

  /// Resume a partner that was parked on a blocking channel operation: advance
  /// it past the instruction and mark it runnable again.
  void unblockAdvance() {
    advanceInstr();
    status = StepResult::RUNNING;
  }

  void traceChannel(hex::OprInstr opr, unsigned slot) {
    out << fmt::format("{:<6d} {:<6d} {:<4} {:<2d} {} channel {}\n", cycles, pc,
                       instrEnumToStr(hex::Instr::OPR), (instr & 0xF),
                       oprInstrEnumToStr(opr), slot);
  }

  /// Execute a channel IN/OUT operation, performing the rendezvous if the
  /// partner is already waiting, otherwise parking this processor (PC is not
  /// advanced so the operation completes when the partner arrives).
  StepResult stepChannel(hex::OprInstr opr) {
    unsigned slot = breg;
    if (slot >= links.size() || links[slot] == nullptr) {
      throw std::runtime_error(fmt::format(
          "processor {}: unwired channel slot {} at pc {:#08x}", id, slot, pc));
    }
    Channel *c = links[slot];
    if (opr == hex::OprInstr::OUT) {
      if (c->state == Channel::State::READER_WAITING) {
        c->reader->areg = areg;
        c->reader->unblockAdvance();
        c->state = Channel::State::IDLE;
        c->reader = nullptr;
        if (tracing) {
          traceChannel(opr, slot);
        }
        advanceInstr();
        return status = StepResult::RUNNING;
      }
      c->state = Channel::State::WRITER_WAITING;
      c->value = areg;
      c->writer = this;
      blockedSlot = slot;
      return status = StepResult::BLOCKED;
    }
    // IN.
    if (c->state == Channel::State::WRITER_WAITING) {
      areg = c->value;
      c->writer->unblockAdvance();
      c->state = Channel::State::IDLE;
      c->writer = nullptr;
      if (tracing) {
        traceChannel(opr, slot);
      }
      advanceInstr();
      return status = StepResult::RUNNING;
    }
    c->state = Channel::State::READER_WAITING;
    c->reader = this;
    blockedSlot = slot;
    return status = StepResult::BLOCKED;
  }

  /// Execute a single instruction. Returns the resulting status.
  StepResult step() {
    if (status != StepResult::RUNNING) {
      return status;
    }
    instr = (memory[pc >> 2] >> ((pc & 0x3) << 3)) & 0xFF;
    oreg = oreg | (instr & 0xF);
    instrEnum = static_cast<hex::Instr>((instr >> 4) & 0xF);
    // Channel operations may block, so they manage the PC themselves.
    if (instrEnum == hex::Instr::OPR) {
      auto oprInstr = static_cast<hex::OprInstr>(oreg);
      if (oprInstr == hex::OprInstr::IN || oprInstr == hex::OprInstr::OUT) {
        return stepChannel(oprInstr);
      }
    }
    lastPC = pc;
    pc = pc + 1;
    if (tracing) {
      trace(instr, instrEnum);
    }
    switch (instrEnum) {
    case hex::Instr::LDAM:
      areg = memory[oreg];
      oreg = 0;
      break;
    case hex::Instr::LDBM:
      breg = memory[oreg];
      oreg = 0;
      break;
    case hex::Instr::STAM:
      memory[oreg] = areg;
      oreg = 0;
      break;
    case hex::Instr::LDAC:
      areg = oreg;
      oreg = 0;
      break;
    case hex::Instr::LDBC:
      breg = oreg;
      oreg = 0;
      break;
    case hex::Instr::LDAP:
      areg = pc + oreg;
      oreg = 0;
      break;
    case hex::Instr::LDAI:
      areg = memory[areg + oreg];
      oreg = 0;
      break;
    case hex::Instr::LDBI:
      breg = memory[breg + oreg];
      oreg = 0;
      break;
    case hex::Instr::STAI:
      memory[breg + oreg] = areg;
      oreg = 0;
      break;
    case hex::Instr::BR:
      pc = pc + oreg;
      oreg = 0;
      break;
    case hex::Instr::BRZ:
      if (areg == 0) {
        pc = pc + oreg;
      }
      oreg = 0;
      break;
    case hex::Instr::BRN:
      if ((int)areg < 0) {
        pc = pc + oreg;
      }
      oreg = 0;
      break;
    case hex::Instr::PFIX:
      oreg = oreg << 4;
      break;
    case hex::Instr::NFIX:
      oreg = 0xFFFFFF00 | (oreg << 4);
      break;
    case hex::Instr::OPR:
      switch (static_cast<hex::OprInstr>(oreg)) {
      case hex::OprInstr::BRB:
        pc = breg;
        oreg = 0;
        break;
      case hex::OprInstr::ADD:
        areg = areg + breg;
        oreg = 0;
        break;
      case hex::OprInstr::SUB:
        areg = areg - breg;
        oreg = 0;
        break;
      case hex::OprInstr::SVC:
        syscall();
        if (tracing) {
          traceSyscall();
        }
        break;
      default:
        throw std::runtime_error("invalid OPR: " + std::to_string(oreg));
      };
      oreg = 0;
      break;
    default:
      throw std::runtime_error("invalid instruction");
    }
    cycles++;
    if (!running) {
      status = StepResult::HALTED;
    }
    return status;
  }

  int run() {
    while (status != StepResult::HALTED &&
           (maxCycles > 0 ? cycles <= maxCycles : true)) {
      step();
    }
    return exitCode;
  }
};

/// Magic number identifying a network container file ("HEXN").
static const uint32_t NETWORK_MAGIC = 0x4E584548;

/// A fixed network of processors connected by point-to-point channels. Boots
/// all processors at reset and steps them round-robin until they all halt.
class System {
  std::vector<std::unique_ptr<Processor>> procs;
  std::vector<std::unique_ptr<Channel>> channels;
  std::istream &in;
  std::ostream &out;
  size_t maxCycles;
  bool tracing = false;
  // Default to truncating character inputs, matching the hardware and xhexb.x
  // behaviour. Tests may enable sign-extension to exercise negative values.
  bool truncateInputs = true;
  int exitCode = 0;
  bool haveExit = false;

public:
  System(std::istream &in, std::ostream &out, size_t maxCycles = 0)
      : in(in), out(out), maxCycles(maxCycles) {}

  void setTracing(bool value) { tracing = value; }
  void setTruncateInputs(bool value) { truncateInputs = value; }

  /// Load a network container, or fall back to a single-processor system if the
  /// file is a plain image (no network magic).
  void loadNetwork(const char *filename) {
    auto container = hexcontainer::read(filename);
    // One processor per image (a plain single image yields one processor).
    for (size_t i = 0; i < container.images.size(); i++) {
      auto &image = container.images[i];
      std::istringstream imageStream(std::string(image.begin(), image.end()),
                                     std::ios::binary);
      addProcessor(imageStream, static_cast<unsigned>(image.size()),
                   static_cast<unsigned>(i));
    }
    // Wire up the channels.
    for (auto &e : container.edges) {
      auto channel = std::make_unique<Channel>();
      procs[e.procA]->setLink(e.slotA, channel.get());
      procs[e.procB]->setLink(e.slotB, channel.get());
      channels.push_back(std::move(channel));
    }
  }

  /// Run the network round-robin until all processors halt. Returns the exit
  /// code of the first processor to exit. Throws on deadlock.
  int run() {
    if (procs.empty()) {
      return 0;
    }
    size_t ticks = 0;
    while (true) {
      // Step every runnable processor once.
      for (auto &p : procs) {
        if (p->getStatus() == StepResult::RUNNING) {
          p->step();
        }
      }
      // Record the exit code of the first (lowest-id) halted processor.
      if (!haveExit) {
        for (auto &p : procs) {
          if (p->getStatus() == StepResult::HALTED) {
            exitCode = p->getExitCode();
            haveExit = true;
            break;
          }
        }
      }
      // Termination and deadlock detection.
      bool allHalted = true;
      bool anyRunning = false;
      for (auto &p : procs) {
        auto s = p->getStatus();
        allHalted = allHalted && s == StepResult::HALTED;
        anyRunning = anyRunning || s == StepResult::RUNNING;
      }
      if (allHalted) {
        return exitCode;
      }
      if (!anyRunning) {
        std::string msg = "deadlock detected:";
        for (size_t i = 0; i < procs.size(); i++) {
          if (procs[i]->getStatus() == StepResult::BLOCKED) {
            msg += fmt::format(" processor {} blocked on channel slot {};", i,
                               procs[i]->getBlockedSlot());
          }
        }
        throw std::runtime_error(msg);
      }
      ticks++;
      if (maxCycles > 0 && ticks > maxCycles) {
        return exitCode;
      }
    }
  }

private:
  void addProcessor(std::istream &image, unsigned imageSize, unsigned id) {
    auto p = std::make_unique<Processor>(in, out, maxCycles);
    p->setId(id);
    p->setTracing(tracing);
    p->setTruncateInputs(truncateInputs);
    p->loadFromStream(image, imageSize);
    procs.push_back(std::move(p));
  }
};

} // End namespace hexsim

#endif // HEX_SIM_HPP
