//
//  Vocabulary.cpp
//  DependencyTreeRNN
//
//  Created by Piotr Mirowski on 05/03/2015.
//  Copyright (c) 2015 Piotr Mirowski. All rights reserved.
//

#include <stdio.h>
#include <cstring>
#include <assert.h>
#include <climits>
#include <cmath>
#include <math.h>
#include <algorithm>
#include "Vocabulary.h"


/**
 * Constructor that reads the vocabulary and classes from the model file.
 */
Vocabulary::Vocabulary(FILE *fi, int sizeVocabulary) {
  // Read the vocabulary, stored in text format as following:
  // index_number count word_token class_number
  // There are tabs and spaces separating the 4 columns
  m_vocabularyStorage.resize(sizeVocabulary);
  for (int a = 0; a < sizeVocabulary; a++) {

    // Read the word index and the word count
    int wordIndex;
    int wordCount;
    fscanf(fi, "%d%d", &wordIndex, &wordCount);
    assert(wordIndex == a);
    m_vocabularyStorage[a].cn = wordCount;
    m_vocabularyStorage[a].prob = 0;

    // Read the word token
    char buffer[2048] = {0};
    if (fscanf(fi, "%s", &buffer))
      m_vocabularyStorage[a].word = buffer;
    std::string word = m_vocabularyStorage[a].word;
    // Read the class index
    int classIndex;
    fscanf(fi, "%d", &classIndex);

    // Store in the vocabulary vector and in the two maps
    m_vocabularyStorage[a].classIndex = classIndex;
    m_mapWord2Index[word] = wordIndex;
    m_mapIndex2Word[wordIndex] = word;
  }
}


/**
 * Save the vocabulary to a model file
 */
void Vocabulary::Save(FILE *fo) {
  // Save the vocabulary, one word per line
  int sizeVocabulary = GetVocabularySize();
  fprintf(fo, "\nVocabulary:\n");
  for (int wordIndex = 0; wordIndex < sizeVocabulary; wordIndex++) {
    int wordCount = m_vocabularyStorage[wordIndex].cn;
    std::string word = m_vocabularyStorage[wordIndex].word;
    int wordClass = m_vocabularyStorage[wordIndex].classIndex;
    fprintf(fo, "%6d\t%10d\t%s\t%d\n",
            wordIndex, wordCount, word.c_str(), wordClass);
  }
}



/**
 * Add a token (word or multi-word entity) to the vocabulary vector
 * and store it in the map from word string to word index
 * and in the map from word index to word string.
 */
int Vocabulary::AddWordToVocabulary(const std::string& word)
{
  int index = SearchWordInVocabulary(word);
  // When a word is unknown, add it to the vocabulary
  if (index == -1) {
    // Initialize the word index, count and probability to 0
    VocabWord w = VocabWord();
    w.word = word;
    w.prob = 0.0;
    w.cn = 1;
    index = static_cast<int>(m_vocabularyStorage.size());
    m_vocabularyStorage.push_back(std::move(w));
    // We need to store the word - index pair in the hash table word -> index
    // but we will rewrite that map later after sorting the vocabulary by frequency
    m_mapWord2Index[word] = index;
    m_mapIndex2Word[index] = word;
  } else {
    // ... otherwise simply increase its count
    m_vocabularyStorage[index].cn++;
  }
  return (index);
}


/**
 * Manually set the word count.
 */
bool Vocabulary::SetWordCount(std::string word, int count) {
  int index = SearchWordInVocabulary(word);
  // When a word is unknown, add it to the vocabulary
  if (index > -1) {
    m_vocabularyStorage[index].cn = count;
    return true;
  } else
    return false;
}


/**
 * Sort the vocabulary by decreasing count of words in the corpus
 * (used for frequency-based word classes, where class 0 contains
 * </s>, class 1 contains {the} or another, most frequent token,
 * class 2 contains a few very frequent tokens, etc...
 */
bool OrderWordCounts(const VocabWord& a, const VocabWord& b) {
  return a.cn > b.cn;
}
void Vocabulary::SortVocabularyByFrequency() {
  // Simply sort the words by frequency, making sure that </s> is first
  int indexEos = SearchWordInVocabulary("</s>");
  int countEos = m_vocabularyStorage[indexEos].cn;
  m_vocabularyStorage[indexEos].cn = INT_MAX;
  std::sort(m_vocabularyStorage.begin(),
            m_vocabularyStorage.end(),
            OrderWordCounts);
  m_vocabularyStorage[indexEos].cn = countEos;

  // Rebuild the the maps of word <-> word index
  m_mapWord2Index.clear();
  m_mapIndex2Word.clear();
  for (int index = 0; index < GetVocabularySize(); index++) {
    std::string word = m_vocabularyStorage[index].word;
    // Add the word to the hash table word -> index
    m_mapWord2Index[word] = index;
    // Add the word to the hash table index -> word
    m_mapIndex2Word[index] = word;
  }
}


/**
 * Return the index of a word in the vocabulary, or -1 if OOV.
 */
int Vocabulary::SearchWordInVocabulary(const std::string& word) const {
  auto i = m_mapWord2Index.find(word);
  if (i == m_mapWord2Index.end()) {
    return -1;
  } else {
    return (i->second);
  }
}


/**
 * Read the classes from a file in the following format:
 * word [TAB] class_index
 * where class index is between 0 and n-1 and there are n classes.
 */
bool Vocabulary::ReadClasses(const std::string &filename)
{
  FILE *fin = fopen(filename.c_str(), "r");
  if (!fin) {
    printf("Error: unable to open %s\n", filename.c_str());
    return false;
  }

  char w[8192];
  int clnum;
  int eos_class = -1;
  int max_class = -1;
  std::set<std::string> words;
  while (fscanf(fin, "%s%d", w, &clnum) != EOF) {
    if (!strcmp(w, "<s>")) {
      printf("Error: <s> should not be in vocab\n");
      return false;
    }

    m_mapWord2Class[w] = clnum;
    m_classes.insert(clnum);
    words.insert(w);

    max_class = (clnum > max_class) ? (clnum) : (max_class);
    eos_class = (std::string(w) == "</s>") ? (clnum) : (eos_class);
  }

  if (eos_class == -1) {
    printf("Error: </s> must be present in the vocabulary\n");
    return false;
  }

  if (m_mapWord2Class.size() == 0) {
    printf("Error: Empty class file!\n");
    return false;
  }

  // </s> needs to have the highest class index because it needs to come first in the vocabulary...
  for (auto si=words.begin(); si!=words.end(); si++) {
    if (m_mapWord2Class[*si] == eos_class) {
      m_mapWord2Class[*si] = max_class;
    } else {
      if (m_mapWord2Class[*si] == max_class) {
        m_mapWord2Class[*si] = eos_class;
      }
    }
  }
  return true;
}



/**
 * Assign words in vocabulary to classes (for hierarchical softmax).
 */
void Vocabulary::AssignWordsToClasses() {
  int sizeVocabulary = GetVocabularySize();
  int sizeClasses = m_numClasses;
  if (m_useClassFile) {
    // Custom-specified classes, provided in a file, were used
    // at training time. There is nothing to do at this point,
    // just copy the class index for each word.
    int cnum = -1;
    int last = -1;
    for (int i = 0; i < sizeVocabulary; i++) {
      if (m_vocabularyStorage[i].classIndex != last) {
        last = m_vocabularyStorage[i].classIndex;
        m_vocabularyStorage[i].classIndex = ++cnum;
      } else {
        m_vocabularyStorage[i].classIndex = cnum;
      }
      // Unused
      m_vocabularyStorage[i].prob = 0.0;
    }
  } else {
    // Frequency-based classes (povey-style)
    // Re-assign classes based on the sqrt(word_count / total_word_count)
    // so that the classes contain equal weight of word occurrences.
    int b = 0;
    for (int i = 0; i < sizeVocabulary; i++) {
      b += m_vocabularyStorage[i].cn;
    }
    double dd = 0;
    for (int i = 0; i < sizeVocabulary; i++) {
      dd += sqrt(m_vocabularyStorage[i].cn/ (double)b);
    }
    double df = 0;
    int a = 0;
    for (int i = 0; i < sizeVocabulary; i++) {
      df += sqrt(m_vocabularyStorage[i].cn / (double)b)/dd;
      if (df > 1) {
        df = 1;
      }
      if (df > (a + 1) / (double)sizeClasses) {
        m_vocabularyStorage[i].classIndex = a;
        if (a < sizeClasses-1) {
          a++;
        }
      } else {
        m_vocabularyStorage[i].classIndex = a;
      }
      // Unused
      m_vocabularyStorage[i].prob = 0.0;
    }
  }

  // Store which words are in which class,
  // using a vector (length number of classes) of vectors (num words in that class)
  m_classWords.resize(sizeClasses);
  for (int i = 0; i < sizeClasses; i++) {
    m_classWords[i].clear();
  }
  for (int i = 0; i < sizeVocabulary; i++) {
    // Assign each word into its class
    int wordClass = m_vocabularyStorage[i].classIndex;
    m_classWords[wordClass].push_back(i);
  }

  // Check that there is no empty class
  for (int i = 0; i < sizeClasses; i++) {
    assert(!(m_classWords[i].empty()));
  }
}
