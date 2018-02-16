#include <demangler/details/node/Type.hh>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <unordered_map>

#include <demangler/details/node/BareFunctionType.hh>
#include <demangler/details/node/BuiltinType.hh>
#include <demangler/details/node/Holder.hh>
#include <demangler/details/node/Name.hh>
#include <demangler/details/node/SourceName.hh>
#include <demangler/details/node/TemplateParam.hh>

namespace demangler
{
namespace details
{
namespace node
{
namespace
{
auto const builtin_types_map = std::unordered_map<char, std::string>{
    {'a', "signed char"}, {'b', "bool"},
    {'c', "char"},        {'d', "double"},
    {'e', "long double"}, {'f', "float"},
    {'g', "__float128"},  {'h', "unsigned char"},
    {'i', "int"},         {'j', "unsigned int"},
    {'l', "long"},        {'m', "unsigned long"},
    {'n', "__int128"},    {'o', "unsigned __int128"},
    {'s', "short"},       {'t', "unsigned short"},
    {'v', "void"},        {'w', "wchar_t"},
    {'x', "long long"},   {'y', "unsigned long long"},
    {'z', "..."},
};

auto const builtin_D_type = std::unordered_map<char, std::string>{
    {'a', "auto"},
    {'c', "decltype(auto)"},
    {'d', "decimal64"},
    {'e', "decimal128"},
    {'f', "decimal32"},
    {'h', "half"},
    {'i', "char32_t"},
    {'n', "decltype(nullptr)"},
    {'s', "char16_t"},
};

auto const cv_qualifiers_map = std::unordered_map<char, std::string>{
    {'K', " const"},
    {'O', "&&"},
    {'P', "*"},
    {'R', "&"},
    {'V', " volatile"},
    {'r', " restrict"},
};
}

Type::Type() noexcept : Node{Node::Type::Type}
{
}

Type::Type(clone_tag, Type const& b)
  : Node{clone_tag{}, b}, cv_qualifiers{b.cv_qualifiers}
{
}

std::ostream& Type::print(PrintOptions const& opt, std::ostream& out) const
{
  if (this->getNodeCount() == 0)
    out << "<empty parameter pack>";
  else if (this->getNodeCount() == 1)
  {
    this->getNode(0)->print(opt, out);
    this->printCVQualifiers(out);
  }
  else
  {
    this->getNode(0)->print(opt, out);
    out << ' ';
    if (!this->cv_qualifiers.empty())
    {
      out << '(';
      this->printCVQualifiers(out);
      out << ')';
    }
    this->getNode(1)->print(opt, out);
  }
  return out;
}

std::unique_ptr<Node> Type::deepClone() const
{
  return std::make_unique<Type>(clone_tag{}, *this);
}

std::unique_ptr<Type> Type::parse(State& s, bool parse_template_args)
{
  auto ret = std::make_unique<Type>();
  auto generates_substitution = true;

  while (!s.empty() &&
         cv_qualifiers_map.find(s.nextChar()) != cv_qualifiers_map.end())
  {
    ret->cv_qualifiers += s.nextChar();
    s.advance(1);
  }
  if (s.empty())
    throw std::runtime_error("Unfinished Type");
  auto it = builtin_types_map.find(s.nextChar());
  if (it != builtin_types_map.end())
  {
    ret->addNode(
        std::make_unique<BuiltinType>(s.symbol.subspan(0, 1), it->second));
    s.advance(1);
    generates_substitution = false;
  }
  else if (s.nextChar() == 'D')
  {
    s.advance(1);
    ret = parseD(s, std::move(ret));
  }
  else if (s.nextChar() == 'u')
  {
    s.advance(1);
    ret->addNode(SourceName::parse(s));
  }
  else if (s.nextChar() == 'S')
  {
    ret->addNode(Name::parse(s));
    // Substitutions do not generate substitutions.
    generates_substitution =
        ret->getNode(0)->getNode(0)->getType() != Node::Type::Substitution;
  }
  else if (s.nextChar() == 'T')
  {
    ret->addNode(TemplateParam::parse(s));
    generates_substitution = false;
  }
  else if (s.nextChar() == 'F')
    ret = parseF(s, std::move(ret));
  else
  {
    auto name = Name::parse(s, parse_template_args);
    ret->addNode(std::move(name));
  }
  if (generates_substitution)
  {
    if (!ret->cv_qualifiers.empty())
    {
      auto tmp = std::string{std::move(ret->cv_qualifiers)};
      ret->cv_qualifiers.clear();
      ret->substitution_made = ret->deepClone();
      s.user_substitutions.emplace_back(ret->substitution_made.get());
      ret->cv_qualifiers = std::move(tmp);
    }
    s.user_substitutions.emplace_back(ret.get());
  }
  return ret;
}

bool Type::isIntegral() const noexcept
{
  auto const contains = [](auto const& container, auto value) {
    return std::find(container.begin(), container.end(), value) !=
           container.end();
  };

  // Pointers and references are integrals.
  if (contains(this->cv_qualifiers, 'P') ||
      contains(this->cv_qualifiers, 'K') ||
      contains(this->cv_qualifiers, 'R') || contains(this->cv_qualifiers, 'O'))
    return true;
  auto const& node = *this->getNode(0);
  if (node.getType() == Node::Type::BuiltinType)
    return static_cast<BuiltinType const&>(node).isIntegral();
  return false;
}

std::unique_ptr<Type> Type::parseD(State& s, std::unique_ptr<Type>&& ret)
{
  auto const it = builtin_D_type.find(s.nextChar());
  if (it != builtin_D_type.end())
  {
    ret->addNode(std::make_unique<BuiltinType>(
        gsl::cstring_span<>{s.symbol.data() - 1, 2}, it->second));
    s.advance(1);
    return std::move(ret);
  }

  switch (s.nextChar())
  {
  case 'T':
  case 't':
    throw std::runtime_error("Unsupported type DT/Dt (decltype expr)");
  case 'p':
    s.advance(1);
    if (s.empty())
      throw std::runtime_error("Empty parameter pack (Dp)");
    return parseDp(s, std::move(ret));
  case 'F':
    throw std::runtime_error("Unsupported type DF (unknown)");
  case 'v':
    throw std::runtime_error("Unsupported type Dv (unknown)");
  case 'x': // Transaction-safe function.
    s.advance(1);
    if (s.empty())
      throw std::runtime_error("Empty Dx");
    if (s.nextChar() != 'F')
      throw std::runtime_error("Transaction-safe type is not a function");
    return parseF(s, std::move(ret));
  default:
    throw std::runtime_error("Unknown D code in type: " + s.toString());
  }
}

std::unique_ptr<Type> Type::parseDp(State& s, std::unique_ptr<Type>&& ret)
{
  if (s.nextChar() == 'T')
  {
    auto const& packnode = TemplateParam::parseParamNode(s);
    if (packnode.getNodeCount() > 0)
      ret->addNode(std::make_unique<Holder>(packnode));
    else
      ret->setEmpty(true);
    s.registerUserSubstitution(ret.get());
  }
  else
    throw std::runtime_error("Invalid symbols in Dp: " +
                             std::string(s.symbol.data()));
  return std::move(ret);
}

std::unique_ptr<Type> Type::parseF(State& s, std::unique_ptr<Type>&& ret)
{
  s.advance(1);
  if (s.empty())
    throw std::runtime_error("Empty function");
  if (s.nextChar() == 'Y')
  {
    // This means extern "C", we don't care.
    s.advance(1);
    if (s.empty())
      throw std::runtime_error("Empty function");
  }

  auto bft = BareFunctionType::parse(s, true, true);
  ret->addNode(bft->retrieveReturnType());
  ret->addNode(std::move(bft));
  return std::move(ret);
}

void Type::printCVQualifiers(std::ostream& out) const
{
  for (auto rit = this->cv_qualifiers.rbegin();
       rit != this->cv_qualifiers.rend();
       ++rit)
  {
    auto const qual = *rit;
    auto const qualit = cv_qualifiers_map.find(qual);
    if (qualit == cv_qualifiers_map.end())
      throw std::logic_error(std::string("Invalid cv-qualifier: ") + qual);
    out << qualit->second;
  }
}
}
}
}
