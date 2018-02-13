#include <demangler/details/node/TemplateParam.hh>

#include <cassert>
#include <limits>

#include <demangler/details/node/SeqId.hh>

namespace demangler
{
namespace details
{
namespace node
{
TemplateParam::TemplateParam() noexcept
  : Node{Type::TemplateParam},
    substitution{nullptr},
    index{std::numeric_limits<unsigned int>::max()}
{
}

TemplateParam::TemplateParam(clone_tag, TemplateParam const& b)
  : Node{clone_tag{}, b}, substitution{b.substitution}, index{b.index}
{
}

std::ostream& TemplateParam::print(PrintOptions const& opt,
                                   std::ostream& out) const
{
  assert(this->substitution);
  this->substitution->print(opt, out);
  return out;
}

std::unique_ptr<Node> TemplateParam::deepClone() const
{
  return std::make_unique<TemplateParam>(clone_tag{}, *this);
}

std::unique_ptr<TemplateParam> TemplateParam::parse(State& s)
{
  assert(s.nextChar() == 'T');
  s.advance(1);
  auto const index = SeqId::parse(s);
  if (s.empty() || s.nextChar() != '_')
    throw std::runtime_error("TemplateParam does not end with '_'");
  s.advance(1);
  auto ret = std::make_unique<TemplateParam>();
  ret->index = index;
  if (!s.awaiting_template)
    ret->performSubstitution(s);
  return ret;
}

Node const& TemplateParam::parseParamNode(State& s)
{
  assert(s.nextChar() == 'T');
  s.advance(1);
  auto const index = SeqId::parse(s);
  if (s.empty() || s.nextChar() != '_')
    throw std::runtime_error("TemplateParam does not end with '_'");
  s.advance(1);
  return *getSubstitutedNodePtr(s, index);
}

void TemplateParam::performSubstitution(State const& s)
{
  this->substitution = this->getSubstitutedNodePtr(s, this->index);
}

Node const* TemplateParam::getSubstitutedNodePtr(State const& s,
                                                 std::size_t idx)
{
  if (idx >= s.template_substitutions.size())
    throw std::runtime_error("TemplateParam with index too high");
  return s.template_substitutions[idx];
}
}
}
}
