#ifndef DEMANGLER_DETAILS_NODE_HH_
#define DEMANGLER_DETAILS_NODE_HH_

#include <memory>
#include <ostream>
#include <vector>

#include <gsl/string_span>

#include <demangler/details/PrintOptions.hh>
#include <demangler/details/State.hh>

namespace demangler
{
namespace details
{
class Node
{
public:
  using string_type = gsl::cstring_span<>;
  enum class Type
  {
    ArrayType,
    BareFunctionType,
    BuiltinSubstitution,
    BuiltinType,
    Constructor,
    Decltype,
    Encoding,
    Expression,
    ExprPrimary,
    LocalName,
    MangledName,
    Name,
    NestedName,
    Number,
    OperatorName,
    Prefix,
    SourceName,
    Substitution,
    TemplateArg,
    TemplateArgs,
    TemplateParam,
    Type,
    UnqualifiedName,
    UnresolvedName,
    UnscopedName,
    UnscopedTemplateName,
    UserSubstitution,
    HolderNode, // Set apart, cause it's not really a node.
  };

  explicit Node(Type t) noexcept;
  Node(Node const& b) noexcept = delete;
  Node(Node&& b) noexcept = default;
  virtual ~Node() noexcept = default;

  Node& operator=(Node const& rhs) noexcept = delete;
  Node& operator=(Node&& rhs) noexcept = default;

  virtual std::ostream& print(PrintOptions const& opt, std::ostream&) const = 0;
  virtual std::unique_ptr<Node> deepClone() const = 0;

  void addNode(std::unique_ptr<Node>&& n);
  Node const* getNode(std::size_t index) const noexcept;
  Node* getNode(std::size_t index) noexcept;
  size_t getNodeCount() const noexcept;
  Type getType() const noexcept;
  void assignTemplateSubstitutions(State const& s);
  bool isEmpty() const noexcept;

  void dumpAST(std::ostream& out,
               size_t indent = 0,
               int maxdepth = 0,
               int depth = 0) const;

protected:
  struct clone_tag
  {
  };

  Node(clone_tag, Node const& b);
  void setEmpty(bool pempty) noexcept;

private:
  std::vector<std::unique_ptr<Node>> children;
  Type type;
  bool empty{false};
};
}
}

#endif /* !DEMANGLER_DETAILS_NODE_HH_ */
