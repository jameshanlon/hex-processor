#include "TestContext.hpp"
#include "hexsim.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

//===---------------------------------------------------------------------===//
// Unit tests for the multi-processor simulator (channels + network container).
//===---------------------------------------------------------------------===//

/// Assemble an assembly source string to a binary file and return its bytes.
static std::vector<char> assembleToBytes(const std::string &asmSrc,
                                         const std::string &name) {
  hexasm::Lexer lexer;
  hexasm::Parser parser(lexer);
  lexer.loadBuffer(asmSrc);
  auto tree = parser.parseProgram();
  hexasm::CodeGen codeGen(tree);
  fs::path path(CURRENT_BINARY_DIRECTORY);
  path /= name;
  codeGen.emitBin(path.c_str());
  std::ifstream file(path.string(), std::ios::binary);
  file.seekg(0, std::ios::end);
  auto size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<char> bytes(size);
  file.read(bytes.data(), size);
  return bytes;
}

/// A single channel wiring edge for a network container.
struct TestEdge {
  uint32_t procA, slotA, procB, slotB;
};

/// Write a network container from a set of image byte-blobs and edges.
static std::string writeContainer(const std::vector<std::vector<char>> &images,
                                  const std::vector<TestEdge> &edges,
                                  const std::string &name) {
  fs::path path(CURRENT_BINARY_DIRECTORY);
  path /= name;
  std::ofstream out(path.string(), std::ios::binary);
  auto writeU32 = [&](uint32_t v) {
    out.write(reinterpret_cast<const char *>(&v), sizeof(uint32_t));
  };
  writeU32(hexsim::NETWORK_MAGIC);
  writeU32(static_cast<uint32_t>(images.size()));
  writeU32(static_cast<uint32_t>(edges.size()));
  for (const auto &e : edges) {
    writeU32(e.procA);
    writeU32(e.slotA);
    writeU32(e.procB);
    writeU32(e.slotB);
  }
  for (const auto &image : images) {
    writeU32(static_cast<uint32_t>(image.size()));
    out.write(image.data(), image.size());
  }
  out.close();
  return path.string();
}

// A processor that sends `value` on channel slot 0 then exits 0.
static std::string senderProgram(int value) {
  return "BR start\n"
         "DATA 16383 # sp\n"
         "start\n"
         "LDAC " +
         std::to_string(value) +
         "\n"        // areg <- value
         "LDBC 0\n"  // breg <- channel slot 0
         "OPR OUT\n" // send areg on channel 0
         "LDAC 0\n"
         "LDBM 1\n" // breg <- sp
         "STAI 2\n" // sp[2] <- 0 (exit code)
         "LDAC 0\n" // areg <- 0 (EXIT)
         "OPR SVC\n";
}

// A processor that reads a word from channel slot 0, writes it to simout,
// exits.
static std::string receiverProgram() {
  return "BR start\n"
         "DATA 16383 # sp\n"
         "start\n"
         "LDBC 0\n"  // breg <- channel slot 0
         "OPR IN\n"  // areg <- value from channel 0
         "LDBM 1\n"  // breg <- sp
         "STAI 2\n"  // sp[2] <- areg (value to write)
         "LDAC 0\n"  // areg <- 0 (stream id)
         "LDBM 1\n"  // breg <- sp
         "STAI 3\n"  // sp[3] <- 0 (simout)
         "LDAC 1\n"  // areg <- 1 (WRITE)
         "OPR SVC\n" //
         "LDAC 0\n"
         "LDBM 1\n"
         "STAI 2\n" // sp[2] <- 0 (exit code)
         "LDAC 0\n" // areg <- 0 (EXIT)
         "OPR SVC\n";
}

// A processor that just reads from channel slot 0 then exits (used for
// deadlock).
static std::string readerOnlyProgram() {
  return "BR start\n"
         "DATA 16383 # sp\n"
         "start\n"
         "LDBC 0\n"
         "OPR IN\n"
         "LDAC 0\n"
         "LDBM 1\n"
         "STAI 2\n"
         "LDAC 0\n"
         "OPR SVC\n";
}

TEST_CASE("[sim_features] single_image_fallback") {
  // A plain image (no network magic) loads as a one-processor system.
  TestContext ctx;
  auto bytes = assembleToBytes(ctx.readFile(ctx.getAsmTestPath("exit0.S")),
                               "sim_exit0.bin");
  fs::path path(CURRENT_BINARY_DIRECTORY);
  path /= "sim_exit0.bin";
  std::istringstream in;
  std::ostringstream out;
  hexsim::System system(in, out);
  system.loadNetwork(path.string().c_str());
  REQUIRE(system.run() == 0);
}

TEST_CASE("[sim_features] rendezvous_writer_first") {
  // Processor order means the writer (proc 0) is stepped before the reader.
  TestContext ctx;
  auto sender = assembleToBytes(senderProgram(65), "sim_sender.bin");
  auto receiver = assembleToBytes(receiverProgram(), "sim_receiver.bin");
  auto file = writeContainer({sender, receiver}, {{0, 0, 1, 0}}, "sim_wf.bin");
  std::istringstream in;
  std::ostringstream out;
  hexsim::System system(in, out);
  system.loadNetwork(file.c_str());
  REQUIRE(system.run() == 0);
  REQUIRE(out.str() == "A");
}

TEST_CASE("[sim_features] rendezvous_reader_first") {
  // Reader (proc 0) is stepped before the writer (proc 1): blocks then resumes.
  TestContext ctx;
  auto receiver = assembleToBytes(receiverProgram(), "sim_receiver2.bin");
  auto sender = assembleToBytes(senderProgram(66), "sim_sender2.bin");
  auto file = writeContainer({receiver, sender}, {{0, 0, 1, 0}}, "sim_rf.bin");
  std::istringstream in;
  std::ostringstream out;
  hexsim::System system(in, out);
  system.loadNetwork(file.c_str());
  REQUIRE(system.run() == 0);
  REQUIRE(out.str() == "B");
}

TEST_CASE("[sim_features] deadlock_detected") {
  // Two processors that both try to read: neither can ever proceed.
  TestContext ctx;
  auto r0 = assembleToBytes(readerOnlyProgram(), "sim_r0.bin");
  auto r1 = assembleToBytes(readerOnlyProgram(), "sim_r1.bin");
  auto file = writeContainer({r0, r1}, {{0, 0, 1, 0}}, "sim_dl.bin");
  std::istringstream in;
  std::ostringstream out;
  hexsim::System system(in, out);
  system.loadNetwork(file.c_str());
  REQUIRE_THROWS_WITH(system.run(),
                      Catch::Matchers::ContainsSubstring("deadlock"));
}

TEST_CASE("[sim_features] unwired_slot_runtime_error") {
  // A sender wired with no channel on slot 0 raises a runtime error.
  TestContext ctx;
  auto sender = assembleToBytes(senderProgram(1), "sim_unwired.bin");
  auto file = writeContainer({sender}, {}, "sim_unwired_net.bin");
  std::istringstream in;
  std::ostringstream out;
  hexsim::System system(in, out);
  system.loadNetwork(file.c_str());
  REQUIRE_THROWS_WITH(system.run(),
                      Catch::Matchers::ContainsSubstring("unwired channel"));
}
