#include <demangler/details/node/Prefix.hh>

#include <cassert>
#include <cctype>
#include <stdexcept>

#include <demangler/details/Utils.hh>
#include <demangler/details/node/OperatorName.hh>
#include <demangler/details/node/Substitution.hh>
#include <demangler/details/node/TemplateArgs.hh>
#include <demangler/details/node/UnqualifiedName.hh>

namespace demangler
{
namespace details
{
namespace node
{
Prefix::Prefix() noexcept : Node{Type::Prefix}
{
}

Prefix::Prefix(clone_tag, Prefix const& b) : Node{clone_tag{}, b}
{
}

std::ostream& Prefix::print(PrintOptions const& opt, std::ostream& out) const
{
  assert(this->getNodeCount() > 0);
  for (auto i = 0u; i < this->getNodeCount(); ++i)
  {
    auto const* node = this->getNode(i);
    if (i && node->getType() != Type::TemplateArgs)
      out << "::";
    node->print(opt, out);
  }
  return out;
}

std::unique_ptr<Node> Prefix::deepClone() const
{
  return std::make_unique<Prefix>(clone_tag{}, *this);
}

std::unique_ptr<Prefix> Prefix::parse(State& s,
                                      string_type ctorname,
                                      bool parse_template)
{
  auto ret = std::make_unique<Prefix>();

  if (s.symbol[0] == 'S' && s.symbol[1] != 't')
    ret->addNode(Substitution::parse(s));
  else
    ret->addNode(UnqualifiedName::parse(s, ctorname));
  if (s.nextChar() == 'I' && parse_template)
    ret->parseTemplate(s);
  return ret;
}

void Prefix::parseTemplate(State& s)
{
  assert(s.nextChar() == 'I');
  this->addNode(TemplateArgs::parse(s));
}

bool Prefix::willHaveReturnType() const noexcept
{
  auto const nodecount = this->getNodeCount();
  if (this->getNode(nodecount - 1)->getType() == Node::Type::TemplateArgs)
  {
    if (nodecount == 1)
      return true;
    // A templated cast operator has no return type.
    auto const* lastprefix = this->getNode(nodecount - 2);
    if (lastprefix->getType() != Node::Type::UnqualifiedName)
      return true;
    auto const* maybe_operatorname = lastprefix->getNode(0);
    if (maybe_operatorname->getType() != Node::Type::OperatorName)
      return true;
    auto const* operatorname =
        static_cast<OperatorName const*>(maybe_operatorname);
    return !operatorname->isCastOperator();
  }
  else
    return false;
}
}
}
}
