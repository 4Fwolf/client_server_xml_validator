/********************************************************************
 * Copyright 2014 Sasha Halchin.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 ********************************************************************/

#include "include/xmlparser.h"

#include <list>
#include <string>

Parser::Parser(std::string _data)
{
  initialize(_data, NULL);
}

Parser::Parser(std::string _data, IParseEvents *pEventHandler)
{
  initialize(_data, pEventHandler);
}

void Parser::initialize(std::string _data, IParseEvents *pEventHandler)
{
#ifndef STATIC_STRING_UTIL
  sUtil.reset(new StringUtil());
#endif

  attr_name = "";
  attr_value = "";
  token = "";

  this->pEventHandler = pEventHandler;
  data = _data;

  root.reset(new Tag("root"));
  pDocument.reset(new Document());

  pDocument->set_root(&(*root));
  idxCurrent = 0;
  state = oldState = psConsume;
  parseMode = pmDOMBuild;
  parse_data();
}

Parser::~Parser()
{
    root.~shared_ptr();
    pDocument.~shared_ptr();
#ifndef STATIC_STRING_UTIL
    sUtil.~shared_ptr();
#endif
}

Document *Parser::loadXML(std::string _data, IParseEvents *pEventHandler)
{
  Parser p(_data, pEventHandler);

  return p.getDocument();
}

void Parser::rewind()
{
  idxCurrent--;
}

int Parser::next_char()
{
  if (idxCurrent >= data.length()) return EOF;

  return data.at(idxCurrent++);
}

int Parser::peek_next_char()
{
  if (idxCurrent >= data.length()) return EOF;

  return data.at(idxCurrent);
}

void Parser::change_state(kParseState newState)
{
  oldState = state;
  state = newState;
  enter_new_state();
}

Tag* Parser::create_tag(std::string name)
{
  Tag *tag = new Tag(name);
  return tag;
}

void Parser::end_tag(std::string tok)
{
  Tag *popped = NULL;

  if (!SUTIL_INVOKE(equals_ignore_case(tagStack.top()->get_name(), tok))) {
    Tag *top = tagStack.top();
    // can be an empty tag, like <br />
    if (top->has_content() == false) {
      popped = tagStack.top();
      tagStack.pop();
    } else {
#ifdef _DEBUG
      printf("WARN: Illegal XML, end-tag has no corrsponding start tag!\n");
#endif
    }
  } else {
    if (!tagStack.empty()) {
      popped = tagStack.top();
      tagStack.pop();
    }
  }

  if (pEventHandler != NULL) {
    pEventHandler->end_tag(reinterpret_cast<ITag*> (popped));
  }

  // In the streamed mode we don't keep tag's
  if ((parseMode == pmStream) && (popped != NULL)) {
    delete popped;
  }
}

void Parser::commit_tag(Tag *pTag)
{
  if (pEventHandler != NULL) {
    pEventHandler->start_tag(reinterpret_cast<ITag*> (pTag));
  }
  // Only store in hierarchy if we are building a 'DOM' tree
  if (parseMode == pmDOMBuild) {
    tagStack.top()->add_child(pTag);
  }

  tagStack.push(pTag);
}

void Parser::enter_new_state()
{
  if (state == psTagContent) {
    commit_tag(tagCurrent);
  }
}

void Parser::parse_data()
{
  char c;
  std::string attr_name = "";
  std::string attr_value = "";
  std::string token = "";

  tagStack.push(&(*root));

  while ((c = next_char()) != EOF) {
    switch (state)
    {
      case psConsume: {
        if (c == '<') {
          int next = peek_next_char();

           if (next == '/') {  // ? '</' - distinguish between token <  and </
            next_char();                     // consume '/'
            change_state(psEndTagStart);
          } else if (next == '!') {
            // Action tag started <!--
            next_char();
            token = "";  // Reset token
            change_state(psCommentStart);
          } else if (next == '?') {
            // Header tag started '<?xml
            next_char();
            token = "";
            change_state(psTagHeader);
          } else {
           change_state(psTagStart);
          }
          token = "";
      } else {
        token += c;
      }
      break;
    }
    case psCommentStart: {  // Make sure we hit '--'
      if ((c == '-') && (peek_next_char() == '-')) {
        next_char();
        change_state(psCommentConsume);
      } else {
#ifdef _DEBUG
        printf("Warning: Illegal start of tag,");
        printf("expected start of comment ('<!--') but found found '<!-'\n");
#endif
        // TODO(Sasha Halchin): If strict, abort here!
        rewind();   // rewind '-'
        rewind();   // rewind '!'
        change_state(psTagStart);
      }
      break;
    }
    case psCommentConsume: {  // parse until -->
      if ((c == '-') && (peek_next_char() == '>')) {
        if (token == "-") {
          next_char();
          change_state(psConsume);
        }
      } else if (c == '-') {
        token = "-";  // Store this in order to track -->
      }
      break;
    }
    case psTagHeader: {  // <?
      if (isspace(c)) {
        // drop them
        tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
        token = "";
        change_state(psTagAttributeName);
      } else {
        token += c;
      }
      break;
    }
    case psTagStart: {  // from psConsume when finding: '<'
      if (isspace(c)) {
        tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
        token = "";
        change_state(psTagAttributeName);
      } else if ((c == '/') && (peek_next_char() == '>')) {
        // catch tags like '<tag/>'
        next_char();  // consume '>'
        tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
        token = "";
        commit_tag(tagCurrent);
        change_state(psConsume);
      } else if (c == '>') {
        tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
        token = "";
        change_state(psTagContent);
      } else {
        token += c;
      }
      break;
    }
    case psEndTagStart: {  // from psConsume when finding: </
      if (isspace(c)) {
        // drop them
      } else if (c == '>') {
        // trim and terminate token
        std::string tmptok(SUTIL_INVOKE(trim(token)));
        end_tag(tmptok);
        // clear token and consume more data
        token = "";
        change_state(psConsume);
      } else {
        token += c;
      }
      break;
    }
    // from psTagStart when finding white-space,
    // from psTagHeader (<?) when finding white-space
    case psTagAttributeName: {
      if (isspace(c)) continue;
      if ((c == '=') && (peek_next_char() == '"')) {
        next_char();  // consume "
        attr_name = token;
        token = "";
        change_state(psTagAttributeValue);
      } else if ((c == '=') && (peek_next_char() == '#')) {
        next_char();  // consume #
        attr_name = token;
        attr_value = "#";
        tagCurrent -> add_attribute(attr_name,  attr_value);
        token = "";
      } else if (c == '>') {  // End of tag
        token = "";
        change_state(psTagContent);
      } else if ((c == '/') && (peek_next_char() == '>')) {
        next_char();
        commit_tag(tagCurrent);
        end_tag(SUTIL_INVOKE(trim(token)));
        token = "";
        change_state(psConsume);
      } else if ((c == '?') && (peek_next_char() == '>')) {
        next_char();
        commit_tag(tagCurrent);
        end_tag(SUTIL_INVOKE(trim(token)));
        token = "";
        change_state(psConsume);
      } else {
        token += c;
      }
      break;
    }
    case psTagAttributeValue: {  // from psTagAttributeName after '='
      if (c == '"') {
        attr_value = token;
        tagCurrent->add_attribute(attr_name, attr_value);
        change_state(psTagAttributeName);
      } else {
        token +=c;
      }
      break;
    }
    case psTagContent: {
      // can't use 'peekNext' since we might have >< which is legal
      if (c == '<') {
        tagCurrent->set_content(SUTIL_INVOKE(trim(token)));
        token = "";
        change_state(psConsume);
        rewind();  // rewind so we will see tag start next time
      } else {
        token += c;
      }
      break;
     }  // case
    }  // switch
  }  // while (!eof)
}  // parse_data

// -- Tag's
Tag::Tag(std::string _name)
{
  set_name(_name);
  content.clear();
}

void Tag::add_attribute(std::string _name, std::string _value)
{
  Attribute *attr = new Attribute();
  attr->set_name(_name);
  attr->set_value(_value);
  attributes.push_back(attr);
}

void Tag::add_child(Tag *tag)
{
  get_children().push_back(tag);
}


bool Tag::has_content()
{
  return (!content.empty());
}

std::string Tag::to_string()
{
  return std::string(name + " ("+content+")");
}

bool Tag::has_attribute(std::string name)
{
  std::list<IAttribute*>::iterator it = attributes.begin();

  for ( ; it != attributes.end(); ++it) {
    IAttribute *pAttribute = *it;
    if (pAttribute->get_name() == name) return true;
  }

  return false;
}

std::string Tag::get_attribute_value(std::string name, std::string defValue)
{
  std::list<IAttribute*>::iterator it = attributes.begin();

  for ( ; it != attributes.end(); ++it) {
    IAttribute *pAttribute = *it;
    if (pAttribute->get_name() == name) return pAttribute->get_value();
  }

  return defValue;
}

std::string Document::indent_string(int depth)
{
  std::string s = "";
  for (int i = 0; i < depth; ++i)
    s += " ";

  return s;
}

// DEBUG HELPER!
void Document::dump_tag_tree(ITag *root, int depth)
{
  std::string indent = indent_string(depth);

  std::list<ITag*> &tags = root->get_children();

  std::list<ITag*>::iterator it = tags.begin();

  while (it != tags.end()) {
    ITag *child = *it;
    dump_tag_tree(child, depth+2);
    ++it;
  }
}

std::string StringUtil::white_spaces_(" \f\n\r\t\v");

void StringUtil::trim_right(std::string& str, const std::string& trim_chars)
{
  std::string::size_type pos = str.find_last_not_of(trim_chars);
  str.erase(pos + 1);
}

void StringUtil::trim_left(std::string& str, const std::string& trim_chars)
{
  std::string::size_type pos = str.find_first_not_of(trim_chars);
  str.erase(0, pos);
}

std::string &StringUtil::trim(std::string& str, const std::string& trim_chars)
{
  trim_right(str, trim_chars);
  trim_left(str, trim_chars);
  return str;
}

std::string StringUtil::to_lower(std::string s)
{
  std::string res = "";

  for (size_t i = 0; i < s.length(); ++i) {
    res += tolower(s.at(i));
  }

  return res;
}

bool StringUtil::equals_ignore_case(std::string a, std::string b)
{
  std::string sa = to_lower(a);
  std::string sb = to_lower(b);
  return (sa == sb);
}

std::string StringUtilStatic::white_spaces_(" \f\n\r\t\v");

ParseStateFunc::ParseStateFunc(std::string _data, IParseEvents *pEventHandler)
{
  initialize(_data, pEventHandler);
}

void ParseStateFunc::state_consume(char c)
{
  if (c == '<') {
    int next = peek_next_char();
    if (next == '/') {  // ? '</' - distinguish between token <  and </
      next_char();  // consume '/'
      change_state(psEndTagStart);
    } else if (next == '!') {
      // Action tag started <!--
      next_char();
      token = "";  // Reset token
      change_state(psCommentStart);
    } else if (next == '?') {
      // Header tag started '<?xml
      next_char();
      token = "";
      change_state(psTagHeader);
    } else {
      change_state(psTagStart);
    }
    token = "";
  } else {
    token += c;
  }
}

void ParseStateFunc::state_comment_start(char c)
{
  if ((c == '-') && (peek_next_char() == '-')) {
    next_char();
    change_state(psCommentConsume);
  } else {
#ifdef _DEBUG
    printf("Warning: Illegal start of tag,");
    printf("expected start of comment ('<!--') but found found '<!-'\n");
#endif
    // TODO(Sasha Halchin): If strict, abort here!
    rewind();   // rewind '-'
    rewind();   // rewind '!'
    change_state(psTagStart);
  }
}

void ParseStateFunc::state_tag_start(char c)
{
  if (isspace(c)) {
    tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psTagAttributeName);
  } else if (c == '/' && peek_next_char() == '>') {
    // catch tags like '<tag/>'
    next_char();  // consume '>'
    tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    commit_tag(tagCurrent);
    change_state(psConsume);
  } else if (c == '>') {
    tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psTagContent);
  } else {
    token += c;
  }
}

void ParseStateFunc::state_end_tag_start(char c)
{
  if (isspace(c)) {
    // drop them
  } else if (c == '>') {
    std::string tmptok(SUTIL_INVOKE(trim(token)));
    token = "";
    end_tag(tmptok);
    change_state(psConsume);
  } else {
    token += c;
  }
}

void ParseStateFunc::state_tag_header(char c)
{
  if (isspace(c)) {
    // drop them
    tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psTagAttributeName);
  } else {
    token += c;
  }
}

void ParseStateFunc::state_comment_consume(char c)
{
  if ((c == '-') && (peek_next_char() == '>')) {
    if (token == "-") {
      next_char();
      change_state(psConsume);
    }
  } else if (c == '-') {
    token = "-";  // Store this in order to track -->
  }
}

void ParseStateFunc::state_attribute_name(char c)
{
  if (isspace(c)) return;

  if ((c == '=') && (peek_next_char() == '"')) {
    next_char();  // consume "
    attr_name = token;
    token = "";
    change_state(psTagAttributeValue);

  } else if ((c == '=') && (peek_next_char() == '#')) {
    next_char();  // consume #
    attr_name = token;
    attr_value = "#";
    tagCurrent->add_attribute(attr_name,  attr_value);
    token = "";
  } else if (c == '>') {  // End of tag
    token="";
    change_state(psTagContent);
  } else if ((c == '/') && (peek_next_char() == '>')) {
    next_char();
    commit_tag(tagCurrent);
    end_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psConsume);
  } else if ((c == '?') && (peek_next_char() == '>')) {
    next_char();
    commit_tag(tagCurrent);
    end_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psConsume);
  } else {
    token += c;
  }
}

void ParseStateFunc::state_attribute_value(char c)
{
  if (c == '"') {
    attr_value = token;
    tagCurrent->add_attribute(attr_name, attr_value);
    change_state(psTagAttributeName);
  } else {
    token += c;
  }
}

void ParseStateFunc::state_tag_content(char c)
{
  // can't use 'peekNext' since we might have >< which is legal
  if (c == '<') {
    tagCurrent->set_content(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psConsume);
    rewind();  // rewind so we will see tag start next time
  } else {
    token += c;
  }
}

void ParseStateFunc::parse_data()
{
  char c;
  tagStack.push(&(*root));

  while ((c = next_char()) != EOF) {
    switch (state)
    {
    case psConsume:
      state_consume(c);
      break;
    case psCommentStart :  // Make sure we hit '--'
      state_comment_start(c);
      break;
    case psCommentConsume:  // parse until -->
      state_comment_consume(c);
      break;
    case psTagHeader :  // <?
      state_tag_header(c);
      break;
    case psTagStart :  // '<'
      state_tag_start(c);
      break;
    case psEndTagStart :  // </
      state_end_tag_start(c);
      break;
    case psTagAttributeName :
      state_attribute_name(c);
      break;
    case psTagAttributeValue :
      state_attribute_value(c);
      break;
    case psTagContent:
      state_tag_content(c);
      break;
    }
  }
}

// -- ParseStateClasses
Tag *ParseStateImpl::create_tag(std::string name)
{
  return pContext->create_tag(name);
}

void ParseStateImpl::end_tag(std::string tok)
{
  pContext->end_tag(tok);
}

void ParseStateImpl::commit_tag(Tag *pTag)
{
  pContext->commit_tag(pTag);
}

void ParseStateImpl::rewind()
{
  pContext->rewind();
}

int ParseStateImpl::next_char()
{
  return pContext->next_char();
}

int ParseStateImpl::peek_next_char()
{
  return pContext->peek_next_char();
}

void ParseStateImpl::change_state(kParseState newState)
{
  pContext->change_state(newState);
}

void StateConsume::enter()
{
  token = "";
}

void StateConsume::consume(char c)
{
  if (c == '<') {
    int next = peek_next_char();

    if (next == '/') {  // ? '</' - distinguish between token <  and </
      next_char();  // consume '/'
      change_state(psEndTagStart);
    } else if (next == '!') {
      // Action tag started <!--
      next_char();
      token = "";  // Reset token
      change_state(psCommentStart);
    } else if (next == '?') {
      // Header tag started '<?xml
      next_char();
      token = "";
      change_state(psTagHeader);
    } else {
      change_state(psTagStart);
    }
    token = "";
  } else {
    token += c;
  }
}

void StateCommentStart::enter()
{
  token = "";
}

void StateCommentStart::consume(char c)
{
  if ((c == '-') && (peek_next_char() == '-')) {
    next_char();
    change_state(psCommentConsume);
  } else {
#ifdef _DEBUG
    printf("Warning: Illegal start of tag,");
    printf("expected start of comment ('<!--') but found found '<!-'\n");
#endif
    // TODO(Sasha Halchin): If strict, abort here!
    rewind();  // rewind '-'
    rewind();  // rewind '!'
    change_state(psTagStart);
  }
}

void StateTagStart::enter()
{
    token = "";
}

void StateTagStart::consume(char c) {
  if (isspace(c)) {
    pContext->tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psTagAttributeName);
  } else if (c == '/' && peek_next_char() == '>') {
    // catch tags like '<tag/>'
    next_char();  // consume '>'
    pContext->tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    commit_tag(pContext->tagCurrent);
    change_state(psConsume);
  } else if (c == '>') {
    pContext->tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psTagContent);
  } else {
    token += c;
  }
}

void StateTagEndStart::enter()
{
  token = "";
}

void StateTagEndStart::consume(char c)
{
  if (isspace(c)) {
    // drop them
  } else if (c == '>') {
    std::string tmptok(SUTIL_INVOKE(trim(token)));
    token = "";
    end_tag(tmptok);
    change_state(psConsume);
  } else {
    token += c;
  }
}

void StateTagHeader::enter()
{
  token = "";
}

void StateTagHeader::consume(char c)
{
  if (isspace(c)) {
    // drop them
    pContext->tagCurrent = create_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psTagAttributeName);
  } else {
    token += c;
  }
}

void StateCommentConsume::enter()
{
  token = "";
}

void StateCommentConsume::consume(char c)
{
  if ((c == '-') && (peek_next_char() == '>')) {
    if (token == "-") {
      next_char();
      change_state(psConsume);
    }
  } else if (c == '-') {
    token = "-";  // Store this in order to track -->
  }
}

void StateAttributeName::enter()
{
  token = "";
}

void StateAttributeName::consume(char c)
{
  if (isspace(c)) return;
  if ((c == '=') && (peek_next_char() == '"')) {
    next_char();  // consume "
    pContext->attr_name = token;
    token = "";
    change_state(psTagAttributeValue);
  } else if ((c == '=') && (peek_next_char() == '#')) {
    next_char();  // consume #
    pContext->attr_name = token;
    pContext->attr_value = "#";
    pContext->tagCurrent->add_attribute(pContext->attr_name,
                                        pContext->attr_value);
    token = "";
  } else if (c == '>') {  // End of tag
    token = "";
    change_state(psTagContent);
  } else if ((c == '/') && (peek_next_char() == '>')) {
    next_char();
    commit_tag(pContext->tagCurrent);
    end_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psConsume);
  } else if ((c == '?') && (peek_next_char() == '>')) {
    next_char();
    commit_tag(pContext->tagCurrent);
    end_tag(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psConsume);
  } else {
    token += c;
  }
}

void StateAttributeValue::enter()
{
  token = "";
}

void StateAttributeValue::consume(char c)
{
  if (c == '"') {
    pContext->attr_value = token;
    pContext->tagCurrent->add_attribute(pContext->attr_name,
                                        pContext-> attr_value);
    change_state(psTagAttributeName);
  } else {
    token += c;
  }
}

void StateTagContent::enter()
{
  token = "";
}

void StateTagContent::consume(char c)
{
  // can't use 'peekNext' since we might have >< which is legal
  if (c == '<') {
    pContext->tagCurrent->set_content(SUTIL_INVOKE(trim(token)));
    token = "";
    change_state(psConsume);
    rewind();  // rewind so we will see tag start next time
  } else {
    token += c;
  }
}

ParseStateClasses::ParseStateClasses(std::string _data,
                                     IParseEvents *pEventHandler)
{
  state_consume.pContext = this;
  state_consume.pContext = this;
  state_tag_start.pContext = this;
  state_comment_start.pContext = this;
  state_tag_end_start.pContext = this;
  state_tag_header.pContext = this;
  state_comment_consume.pContext = this;
  state_attribute_name.pContext = this;
  state_attribute_value.pContext = this;
  state_tag_content.pContext = this;
  pState = dynamic_cast<IParseState*> (&state_consume);

  initialize(_data, pEventHandler);
}

void ParseStateClasses::change_state(kParseState newState) {
  if (pState != NULL) pState->leave();

  switch (newState) {
    case psConsume:
        pState = dynamic_cast<IParseState*> (&state_consume); break;
    case psTagStart:
        pState = dynamic_cast<IParseState*> (&state_tag_start); break;
    case psCommentStart:
        pState = dynamic_cast<IParseState*> (&state_comment_start); break;
    case psEndTagStart:
        pState = dynamic_cast<IParseState*> (&state_tag_end_start); break;
    case psTagHeader:
        pState = dynamic_cast<IParseState*> (&state_tag_header); break;
    case psCommentConsume:
        pState = dynamic_cast<IParseState*> (&state_comment_consume); break;
    case psTagAttributeName:
        pState = dynamic_cast<IParseState*> (&state_attribute_name); break;
    case psTagAttributeValue:
        pState = dynamic_cast<IParseState*> (&state_attribute_value); break;
    case psTagContent:
        pState = dynamic_cast<IParseState*> (&state_tag_content); break;
    default:
        pState = NULL;
  }
  // State tracking variable managed by base class
  Parser::change_state(newState);
  if (pState != NULL) pState->enter();
}

void ParseStateClasses::initialize(std::string _data,
                                   IParseEvents *pEventHandler)
{
  Parser::initialize(_data, pEventHandler);
}

void ParseStateClasses::parse_data()
{
  char c;
  tagStack.push(&(*root));

  while ((c = next_char()) != EOF) {
    if (pState != NULL) {
      pState->consume(c);
    }
  }
}
