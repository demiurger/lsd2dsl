#pragma once

#include "BitStream.h"
#include <string>
#include <deque>
#include <functional>

namespace dictlsd {

struct ExtPair {
    unsigned char idx;
    char16_t chr;
};

struct CharInfo {
    bool sorted;
    bool escaped;
    char16_t chr;
    bool operator==(const CharInfo& other) const;
};

typedef std::vector<CharInfo> CharVec;
typedef std::function<bool(CharVec const& chars, CharVec& left, CharVec& middle, CharVec& right)> Matcher;

class IDictionaryDecoder;
class ArticleHeading {
    std::vector<CharInfo> _chars;
    std::deque<ExtPair> _pairs;
    std::u16string _text;
    bool _hasExtText;
    std::u16string _extText;
    unsigned _reference;
    void makeExtTextFromChars();
    friend void collapseVariants(std::vector<ArticleHeading> &);
    friend bool tryCollapse(ArticleHeading& variant1,
                            ArticleHeading& variant2,
                            ArticleHeading& collapsed,
                            CharVec const& beforeMiddle,
                            CharVec const& afterMiddle,
                            Matcher matcherA,
                            Matcher matcherB);
public:
    ArticleHeading();
    bool Load(IDictionaryDecoder& decoder,
              IBitStream& bstr,
              std::u16string& knownPrefix);
    std::u16string text() const;
    std::u16string extText();
    unsigned articleReference() const;
};

void collapseVariants(std::vector<ArticleHeading> &headings);
void groupHeadingsByReference(std::vector<ArticleHeading>& headings);
typedef std::vector<ArticleHeading>::iterator ArticleHeadingIter;
void foreachReferenceSet(std::vector<ArticleHeading>& groupedHeadings,
                         std::function<void(ArticleHeadingIter, ArticleHeadingIter)> func);

}
