#pragma once
//------------------------------------------------------------------------------
//
//   Copyright 2018-2020 Fetch.AI Limited
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
//
//------------------------------------------------------------------------------

#include "core/byte_array/byte_array.hpp"
#include "core/macros.hpp"

#include <numeric>
#include <utility>
#include <vector>

#include <iostream>

namespace fetch {
namespace http {

struct HtmlTags
{
  using Tag     = byte_array::ConstByteArray;
  using Content = byte_array::ConstByteArray;
  using Params  = std::vector<std::pair<Tag, Content>>;

  HtmlTags() = default;
  Content operator()(Tag const &tag, Params const &params) const;
  Content operator()(Tag const &tag, Params const &params, Content content) const;
};

struct TopLevelContentTag
{
};

static TopLevelContentTag top_level_content;

FETCH_UNUSED_IN_NAMESPACE(top_level_content)

template <class TP>
class TaggedTree
{
public:
  using TaggingPolicy = TP;
  using Tag           = typename TaggingPolicy::Tag;
  using Params        = typename TaggingPolicy::Params;
  using Content       = typename TaggingPolicy::Content;
  using Children      = std::vector<TaggedTree>;

  explicit TaggedTree(TaggingPolicy policy = {})
    : tagging_policy_(std::move(policy))
  {}

  explicit TaggedTree(Tag tag, Params params = {}, TaggingPolicy policy = {})
    : tagging_policy_(std::move(policy))
    , tag_(std::move(tag))
    , params_(std::move(params))
  {}

  TaggedTree(Tag tag, Content content, Params params = {}, TaggingPolicy policy = {})
    : tagging_policy_(std::move(policy))
    , tag_(std::move(tag))
    , params_(std::move(params))
    , content_(std::move(content))
  {}

  TaggedTree(Tag tag, Children children, Params params = {}, TaggingPolicy policy = {})
    : tagging_policy_(std::move(policy))
    , tag_(std::move(tag))
    , params_(std::move(params))
    , children_(std::move(children))
  {}

  TaggedTree(Tag tag, Content content, Children children, Params params = {},
             TaggingPolicy policy = {})
    : tagging_policy_(std::move(policy))
    , tag_(std::move(tag))
    , params_(std::move(params))
    , content_(std::move(content))
    , children_(std::move(children))
  {}

  TaggedTree(TopLevelContentTag /*top_level_content_tag*/, Content content, Children children = {})
    : content_(std::move(content))
    , children_(std::move(children))
  {}

  TaggedTree(TaggedTree &&) noexcept = default;
  TaggedTree(TaggedTree const &)     = default;

  ~TaggedTree() = default;

  TaggedTree &operator=(TaggedTree &&) noexcept = default;
  TaggedTree &operator=(TaggedTree const &) = default;

  TaggingPolicy GetTaggingPolicy() const
  {
    return tagging_policy_;
  }
  TaggedTree &SetTaggingPolicy(TaggingPolicy tagging_policy)
  {
    tagging_policy_ = std::move(tagging_policy);
    return *this;
  }

  Tag GetTag() const
  {
    return tag_;
  }
  TaggedTree &SetTag(Tag tag)
  {
    tag_ = std::move(tag);
    return *this;
  }

  Params GetParams() const
  {
    return params_;
  }
  TaggedTree &SetParams(Params params)
  {
    params_ = std::move(params);
    return *this;
  }

  Content GetContent() const
  {
    return content_;
  }
  TaggedTree &SetContent(Content content)
  {
    content_ = std::move(content);
    return *this;
  }

  Children GetChildren() const
  {
    return children_;
  }
  TaggedTree &SetChildren(Children children)
  {
    children_ = std::move(children);
    return *this;
  }

  void push_back(TaggedTree child)
  {
    children_.push_back(std::move(child));
  }

  template <class... Args>
  void emplace_back(Args &&... args)
  {
    children_.emplace_back(std::forward<Args>(args)...);
  }

  Content Render() const;

private:
  TaggingPolicy tagging_policy_;
  Tag           tag_;
  Params        params_;
  Content       content_;
  Children      children_;
};

using HtmlTree    = TaggedTree<HtmlTags>;
using HtmlTag     = HtmlTree::Tag;
using HtmlParams  = HtmlTree::Params;
using HtmlContent = HtmlTree::Content;
using HtmlNodes   = HtmlTree::Children;

template <class... Elements>
HtmlTree HtmlBody(Elements... elements)
{
  return HtmlTree("html", HtmlNodes{HtmlTree("body", std::forward<Elements>(elements)...)});
}

template <class TaggingPolicy>
typename TaggedTree<TaggingPolicy>::Content TaggedTree<TaggingPolicy>::Render() const
{
  if (children_.empty() && content_.empty())
  {
    return tagging_policy_(tag_, params_);
  }

  Content content = std::accumulate(children_.begin(), children_.end(), Content{},
                                    [](auto accum, auto const &child) {
                                      return std::move(accum) + child.Render();
                                    }) +
                    content_;

  return tagging_policy_(tag_, params_, content);
}

}  // namespace http
}  // namespace fetch
