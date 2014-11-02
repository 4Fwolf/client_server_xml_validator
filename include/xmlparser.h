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

#ifndef TRLWO_1286_INCLUDE_XMLPARSER_H_
#define TRLWO_1286_INCLUDE_XMLPARSER_H_

#include <stdio.h>
#include <string>
#include <list>
#include <stack>
#include <memory>

    // -- start config
#define XML_PARSER_STATIC_STRING_UTIL
    // -- end config

#ifdef XML_PARSER_STATIC_STRING_UTIL
#define SUTIL_INVOKE(__x__) (StringUtilStatic::__x__)
#else
#define SUTIL_INVOKE(__x__) (sUtil->__x__)
#endif

    //
    // Class for work with strings
    //
    class StringUtil
    {
      static std::string white_spaces_;

     public:
      // Delete right trimmer
      void trim_right(std::string& str,
                      const std::string& trim_chars = white_spaces_);

      // Delete left trimmer
      void trim_left(std::string& str,
                     const std::string& trim_chars = white_spaces_);

      // Delete all trimmers
      std::string &trim(std::string& str,
                        const std::string& trim_chars = white_spaces_);

      // String to lowercase
      std::string to_lower(std::string s);

      // Comparing strings in lowercase
      bool equals_ignore_case(std::string a, std::string b);
    };

    //
    // Class for work with strings
    //
    class StringUtilStatic
    {
      static std::string white_spaces_;

     public:
      // Delete right trimmer
      __inline static void trim_right(std::string& str,
                                 const std::string& trim_chars = white_spaces_)
      {
        std::string::size_type pos = str.find_last_not_of(trim_chars);
        str.erase(pos + 1);
      }

      // Delete left trimmer
      __inline static void trim_left(std::string& str,
                                 const std::string& trim_chars = white_spaces_)
      {
        std::string::size_type pos = str.find_first_not_of(trim_chars);
        str.erase(0, pos);
      }

      // Delete all trimmers
      __inline static std::string &trim(std::string& str,
                                 const std::string& trim_chars = white_spaces_)
      {
        trim_right(str, trim_chars);
        trim_left(str, trim_chars);
        return str;
      }

      // String to lowercase
      __inline static std::string to_lower(std::string s)
      {
        std::string res = "";

        for (size_t i = 0; i < s.length(); ++i)
          res += tolower(s.at(i));

        return res;
      }

      // Comparing strings in lowercase
      __inline static bool equals_ignore_case(std::string a, std::string b)
      {
        std::string sa = to_lower(a);
        std::string sb = to_lower(b);
        return (sa == sb);
      }
    };

    //
    // Class for work with attributes
    // (Here are the public interfaces)
    //
    class IAttribute {
     public:
      // Get name of attribute
      virtual std::string& get_name() = 0;
      // Get value of attribute
      virtual std::string& get_value() = 0;
    };

    //
    // Class for work with tags
    //
    class ITag {
     public:
      // Tag has content?
      virtual bool has_content() = 0;
      // Get name of tag
      virtual std::string &get_name() = 0;
      // Get content of tag
      virtual std::string &get_content() = 0;
      // Put name and content to one string
      virtual std::string to_string() = 0;
      // Tag has attribute?
      virtual bool has_attribute(std::string name) = 0;
      // Get value of attribute
      virtual std::string get_attribute_value(std::string name,
                                              std::string defValue) = 0;
      // List of attributes
      virtual std::list <IAttribute*> &get_attributes() = 0;
      // List of child tags
      virtual std::list <ITag*> &get_children() = 0;
    };

    //
    // Class for work with documents
    //
    class IDocument {
     public:
      // Get root of document
      virtual ITag *get_root() = 0;
    };

    //
    // Class for work with parser ivents
    //
    class IParseEvents{
     public:
      // Start tags
      virtual void start_tag(ITag *pTag) = 0;
      // End tags
      virtual void end_tag(ITag *pTag) = 0;
      // Content tags
      virtual void content_tag(ITag *pTag,
                               const std::string &content) = 0;
    };

    //
    // Class for work with attributes
    // (Internal parser classes and default implementations)
    //
    class Attribute : public IAttribute {
     private:
      std::string name;
      std::string value;

     public:
      Attribute() {}

      virtual std::string &get_name() { return name; }
      void set_name(std::string &_name) { name = _name; }

      virtual std::string& get_value() { return value; }
      void set_value(std::string &_value) {value = _value; }
    };

    //
    // Class for work with tags
    //
    class Tag : public ITag {
     private:
      std::string name;
      std::string content;

      std::list<IAttribute*> attributes;
      std::list<ITag*> children;

     public:
      explicit Tag(std::string _name);
      virtual ~Tag() {}

      virtual bool has_content();
      virtual std::string to_string();

      void add_attribute(std::string _name,
                         std::string _value);
      void add_child(Tag *tag);

      virtual std::string &get_name() { return name; }
      void set_name(std::string &_name) { name = _name; }

      virtual std::string &get_content() { return content; }
      void set_content(std::string &_content) { content = _content; }

      bool has_attribute(std::string name);
      std::string get_attribute_value(std::string name,
                                      std::string defValue);


      virtual std::list<IAttribute*> &get_attributes() { return attributes; }
      virtual std::list<ITag*> &get_children() { return children; }
    };


    //
    // Class for work with documents
    //
    class Document {
      Tag *root;

     public:
      Document() { root = NULL; }
      virtual ~Document() {}

      virtual ITag *get_root() { return root; }
      void set_root(Tag *pRoot) { root = pRoot; }
      // Debug helper
      void dump_tag_tree(ITag *root, int depth);

     private:
      std::string indent_string(int depth);
    };

    // Enum for keeping tag information
    enum kParseState {
      psConsume,
      psTagStart,
      psEndTagStart,
      psTagAttributeName,
      psTagAttributeValue,
      psTagContent,
      psTagHeader,
      psCommentStart,
      psCommentConsume,
    };

    enum kParseMode {
      pmStream,
      pmDOMBuild,
    };

    //
    // Had to do this in order to try out a few things without to much changes
    // context is an internal class
    //
    class IParseContext {
     public:
      // Create tag
      virtual Tag *create_tag(std::string name) = 0;
      // End tag
      virtual void end_tag(std::string tok) = 0;
      // Commit tag
      virtual void commit_tag(Tag *pTag) = 0;
      // Rewinding
      virtual void rewind() = 0;
      // Next char
      virtual int next_char() = 0;
      // Peek next char
      virtual int peek_next_char() = 0;
      // Change parse state
      virtual void change_state(kParseState newState) = 0;
      Tag *tagCurrent;
      std::string attr_name;
      std::string attr_value;
    };

    //
    // Actual parser
    //
    class Parser : public IParseContext {
     public:
      explicit Parser(std::string _data);
      Parser(std::string _data,
             IParseEvents *pEventHandler);
      virtual ~Parser();

      // Load XML document
      static Document *loadXML(std::string _data,
                               IParseEvents *pEventHandler = NULL);

      Document* getDocument() { return &(*pDocument); }

     protected:
      Parser() {}
      // Initialize parser
      virtual void initialize(std::string _data,
                              IParseEvents *pEventHandler);

      // Parse data
      virtual void parse_data();
      // Change parse state
      virtual void change_state(kParseState newState);

      // Create tag
      Tag *create_tag(std::string name);
      // End tag
      void end_tag(std::string tok);
      // Commit tag
      void commit_tag(Tag *pTag);

      // Rewinding
      void rewind();
      // Next char
      int next_char();
      // Peek next char
      int peek_next_char();

      // Enter to new parse state
      void enter_new_state();

      // Pointers to document and root of document
      std::shared_ptr<StringUtil> sUtil;
      std::shared_ptr<Tag>        root;
      std::shared_ptr<Document>   pDocument;

      // Parse state info
      kParseState state;
      kParseState oldState;
      kParseMode  parseMode;

      std::stack<Tag*> tagStack;
      int              idxCurrent;
      std::string      data;
      IParseEvents     *pEventHandler;
      // Parser variables
      std::string token;
    };

    // --------------------- Main stuff ends here,
    // rest is just for performance testing of various calling techniques

    //
    // Class where each state is implemented in a seprate function
    //
    class ParseStateFunc : public Parser {
     public:
      ParseStateFunc(std::string _data,
                     IParseEvents *pEventHandler);

      virtual void parse_data();
      __inline void state_consume(char c);
      __inline void state_comment_start(char c);
      __inline void state_tag_start(char c);
      __inline void state_end_tag_start(char c);
      __inline void state_tag_header(char c);
      __inline void state_comment_consume(char c);
      __inline void state_attribute_name(char c);
      __inline void state_attribute_value(char c);
      __inline void state_tag_content(char c);
    };


    //
    // -- Classes related to the state-class parser implementation
    // i.e each state has it's own class..

    class IParseState {
     public:
      virtual void enter() = 0;
      virtual void consume(char c) = 0;
      virtual void leave() = 0;
    };

    class ParseStateImpl : public IParseState {
     public:
      IParseContext *pContext;
      std::string token;

      virtual void enter() {}
      virtual void consume(char c) {}
      virtual void leave() {}

      // -- helpers
      Tag *create_tag(std::string name);
      void end_tag(std::string tok);
      void commit_tag(Tag *pTag);

      void rewind();
      int next_char();
      int peek_next_char();
      void change_state(kParseState newState);
    };

    class StateConsume : public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateCommentStart : public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateTagStart : public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateTagEndStart : public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateTagHeader : public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateCommentConsume: public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateAttributeName: public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateAttributeValue: public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };

    class StateTagContent: public ParseStateImpl {
     public:
      virtual void enter();
      virtual void consume(char c);
    };


    class ParseStateClasses : public Parser {
     private:
      IParseState           *pState;
      StateConsume          state_consume;
      StateCommentStart     state_comment_start;

      StateTagStart         state_tag_start;
      StateTagEndStart      state_tag_end_start;
      StateTagHeader        state_tag_header;

      StateCommentConsume   state_comment_consume;
      StateAttributeName    state_attribute_name;
      StateAttributeValue   state_attribute_value;
      StateTagContent       state_tag_content;

     public:
      ParseStateClasses(std::string _data,
                        IParseEvents *pEventHandler);

      virtual void change_state(kParseState newState);

      virtual void initialize(std::string _data,
                              IParseEvents *pEventHandler);

      virtual void parse_data();
    };

    class ParseEventTracker : public IParseEvents
    {
     public:
        int start_tags_;
        int end_tags_;
        int content_tags_;
        bool valid_;

      ParseEventTracker():
          start_tags_(0),
          end_tags_(0),
          content_tags_(0),
          valid_(false){}
      ~ParseEventTracker() = default;

      virtual void start_tag(ITag *pTag)
      {
        ++start_tags_;
      }

      virtual void end_tag(ITag *pTag)
      {
        ++end_tags_;

        if (start_tags_ == end_tags_) {
            valid_ = true;
        } else {
            valid_ = false;
        }
      }

      virtual void content_tag(ITag *pTag, const std::string &content)
      {
        ++content_tags_;
      }

      bool result()
      {
          return valid_;
      }
    };
#endif  // TRLWO_1286_INCLUDE_XMLPARSER_H_
