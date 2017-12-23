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
    BareFunctionType,
    BuiltinSubstitution,
    BuiltinType,
    Constructor,
    Encoding,
    MangledName,
    Name,
    NestedName,
    OperatorName,
    Prefix,
    SourceName,
    Substitution,
    TemplateArg,
    TemplateArgs,
    TemplateParam,
    Type,
    UnqualifiedName,
    UnscopedName,
    UnscopedTemplateName,
    UserSubstitution,
  };

  explicit Node(Type t) noexcept;
  Node(Node const& b) noexcept = delete;
  Node(Node&& b) noexcept = default;
  ~Node() noexcept = default;

  Node& operator=(Node const& rhs) noexcept = delete;
  Node& operator=(Node&& rhs) noexcept = default;

  virtual std::ostream& print(PrintOptions const& opt, std::ostream&) const = 0;
  virtual std::unique_ptr<Node> deepClone() const = 0;

  void addNode(std::unique_ptr<Node>&& n);
  Node const* getNode(std::size_t index) const noexcept;
  size_t getNodeCount() const noexcept;
  Type getType() const noexcept;

protected:
  struct clone_tag
  {
  };

  Node(clone_tag, Node const& b);

private:
  std::vector<std::unique_ptr<Node>> children;
  Type type;
};
}
}

#endif /* !DEMANGLER_DETAILS_NODE_HH_ */
