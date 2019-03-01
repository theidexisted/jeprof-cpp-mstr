#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

std::string FunctionName(size_t i) { return "fun_" + std::to_string(i); }

std::string FunctionName() {
  static size_t n = 0;
  n++;
  return FunctionName(n);
}

class Function {
 public:
  Function(std::string name) : name_(name) {
    // TODO: use variable size here
    statements_.emplace_back("void* ptr = malloc(64);");
  }
  std::string Name() const { return name_; }
  const std::vector<std::string> &Statements() const { return statements_; }
  const std::set<std::string> &Visited() const { return visited_; }
  void Visit(std::string fun) { visited_.insert(fun); }
  bool CanVisit(std::string fun) {
    return fun != name_ && visited_.find(fun) == visited_.end();
  }

 private:
  std::string name_;
  std::vector<std::string> statements_;
  std::set<std::string> visited_;
};

class Program {
 public:
  friend class Printer;
  Program() = default;
  void RandomPrimGenerate(size_t size, int elongation);
  void AddLine(size_t from, size_t to);

 private:
  int WNextRand(int n, int w);
  Function &GetOrCreate(size_t n);
  std::map<std::string, Function> program_;
};

int Program::WNextRand(int n, int w) {
  assert(w != 0);
  bool neg = (w < 0);
  if (neg) w = -w;

  std::random_device rd;
  std::uniform_int_distribution<int> dist(0, n - 1);
  std::vector<int> randomList(w);
  std::generate(std::begin(randomList), std::end(randomList),
                [&dist, &rd] { return dist(rd); });

  if (neg) {
    return *std::min_element(std::begin(randomList), std::end(randomList));
  }

  return *std::max_element(std::begin(randomList), std::end(randomList));
}

Function &Program::GetOrCreate(size_t n) {
  const std::string name = FunctionName(n);
  auto it = program_.find(name);
  if (it != std::end(program_)) return it->second;

  return program_.emplace(name, Function(name)).first->second;
}

void Program::RandomPrimGenerate(size_t size, int elongation) {
  for (size_t v = 1; v < size; ++v) {
    size_t parent = WNextRand(v, elongation);
    size_t x = v;
    if (parent > x) std::swap(parent, x);
    GetOrCreate(parent).Visit(GetOrCreate(x).Name());
  }
}

class Printer {
 public:
  void PrintHeader(const Program &);
  void PrintDeclare(const Program &);
  void PrintDefinition(const Program &);
  void PrintMain(const Program &);
  void Print(const Program &);

 private:
};

void Printer::PrintHeader(const Program &program) {
  const std::string header =
      "#include <iostream>\n"
      "#include <memory>\n"
      "#include <thread>\n"
      "#include <random>\n"
      "#include <vector>\n"
      "#include <cstdlib>\n"
      "#include <functional>\n";

  std::cout << header << std::endl;
}

void Printer::PrintDeclare(const Program &program) {
  auto DeclareStatment = [](const std::string &name) {
    return std::string("static void ").append(name).append("();");
  };
  for (const auto &kv : program.program_) {
    std::cout << DeclareStatment(kv.second.Name()) << std::endl;
  }
  std::cout << std::endl;
}

void Printer::PrintDefinition(const Program &program) {
  const auto Definition = [](const std::string &name) {
    return std::string("void __attribute__ ((noinline))")
        .append(name)
        .append("() {");
  };
  const auto CallStatement = [](const std::string &name) {
    return std::string(name).append("();");
  };
  static const std::string kIndent = "  ";

  for (const auto &kv : program.program_) {
    std::cout << Definition(kv.second.Name()) << std::endl;
    for (const auto &s : kv.second.Statements()) {
      std::cout << kIndent << s << std::endl;
    }
    for (const auto &v : kv.second.Visited()) {
      std::cout << kIndent << CallStatement(v) << std::endl;
    }

    std::cout << "}" << std::endl;
  }
  std::cout << std::endl;
}

void Printer::PrintMain(const Program &program) {
  std::string functionTemplate =
      "int main(int argc, char* argv[]) {\n"
      "  std::vector<std::function<void()>> functions = {\n"
      "    %s\n"
      "  };\n"
      "  std::vector<std::thread> threads;\n"
      "  const size_t kRunLoops = %d;\n"
      "  for (size_t i = 0;i < std::thread::hardware_concurrency(); ++i) {\n"
      "    threads.emplace_back(std::thread([&functions, kRunLoops](){\n"
      "      std::random_device rd;\n"
      "      std::uniform_int_distribution<int> dist(0, functions.size() - "
      "1);\n"
      "      for (size_t j = 0; j < kRunLoops; ++j) {\n"
      "        functions[dist(rd)]();\n"
      "      }\n"
      "    }));\n"
      "  }\n"
      "  for (auto& t :threads) t.join();\n"
      "\n"
      "  return 0;\n"
      "}";

  std::vector<std::string> functionCallStatments;
  std::transform(std::begin(program.program_), std::end(program.program_),
                 std::back_inserter(functionCallStatments),
                 [](const decltype(program.program_)::value_type &kv) {
                   return kv.second.Name();
                 });
  const static std::string kTailIndent = ",\n    ";
  std::string functionList =
      boost::algorithm::join(functionCallStatments, kTailIndent);

  // TODO: Use variable here
  std::cout << (boost::format(functionTemplate) % functionList.c_str() % 100000)
            << std::endl;
}

void Printer::Print(const Program &program) {
  PrintHeader(program);
  PrintDeclare(program);
  PrintDefinition(program);
  PrintMain(program);
}

void help() { std::cout << "./a.out NodeNum" << std::endl; }

int main(int argc, char *argv[]) {
  if (argc != 2) {
    help();
    return 0;
  }

  int nodes = 0;
  try {
    nodes = std::stoi(argv[1]);
  } catch (...) {
    help();
    return 0;
  }

  Program program;
  program.RandomPrimGenerate(nodes, 3);

  Printer printer;
  printer.Print(program);

  return 0;
}