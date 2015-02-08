// Copyright (c) 2014-2015 Piotr Mirowski. All rights reserved.
//                         piotr.mirowski@computer.org
//
// Based on code by Geoffrey Zweig and Tomas Mikolov
// for the Recurrent Neural Networks Language Model (RNNLM) toolbox
//
// Recurrent neural network based statistical language modeling toolkitsize
// Version 0.3f
// (c) 2010-2012 Tomas Mikolov (tmikolov@gmail.com)
// Extensions from 0.3e to 0.3f version done at Microsoft Research
//
// This code implements the following paper:
//   Tomas Mikolov and Geoffrey Zweig
//   "Context Dependent Recurrent Neural Network Language Model"
//   Microsoft Research Technical Report MSR-TR-2012-92 July 27th, 2012
//   IEEE Conference on Spoken Language Technologies
//   http://research.microsoft.com/apps/pubs/default.aspx?id=176926

#ifndef __DependencyTreeRNN____RnnDependencyTreeLib__
#define __DependencyTreeRNN____RnnDependencyTreeLib__

#include "RnnLib.h"
#include "RnnTraining.h"
#include "CorpusUnrollsReader.h"

class RnnTreeLM : public RnnLMTraining {
public:
  
  /**
   * Constructor for testing the model
   */
  RnnTreeLM(const std::string &filename, bool debugMode)
  // We load the RNN from filename
  : RnnLMTraining(filename, debugMode),
  m_typeOfDepLabels(0) {
  }
  
  /**
   * Constructor for training the model
   */
  RnnTreeLM(const std::string &filename, bool debugMode, bool isBinary)
  // We do not load the RNN, simply set its filename
  : RnnLMTraining(filename, debugMode, isBinary),
  m_typeOfDepLabels(0) {
  }
  
public:
  
  /**
   * Before learning the RNN model, we need to learn the vocabulary
   * from the corpus. Note that the word classes may have been initialized
   * beforehand using ReadClasses. Computes the unigram distribution
   * of words from a training file, assuming that the existing vocabulary
   * is empty.
   */
  bool LearnVocabularyFromTrainFile();
  
  /**
   * Return the number of labels (features) used in the dependency parsing.
   */
  int GetLabelSize() const {
    return static_cast<int>(m_mapLabel2Index.size());
  }
  
  /**
   * Set the mode of the dependency labels:
   * 0: no dependency labels used
   * 1: dependency labels concatenated to the word
   * 0: dependency labels used as features in the feature vector
   */
  void SetDependencyLabelType(int type) {
    m_typeOfDepLabels = type;
  }

  /**
   * Set the minimum number of word occurrences
   */
  void SetMinWordOccurrence(int val) {
    m_corpusVocabulary.SetMinWordOccurrence(val);
    m_corpusTrain.SetMinWordOccurrence(val);
    m_corpusValidTest.SetMinWordOccurrence(val);
  }
  
  /**
   * Add a book to the training corpus
   */
  void AddBookTrain(const std::string &filename) {
    m_corpusVocabulary.AddBookFilename(filename);
    m_corpusTrain.AddBookFilename(filename);
  }
  
  /**
   * Add a book to the test/validation corpus
   */
  void AddBookTestValid(const std::string &filename) {
    m_corpusValidTest.AddBookFilename(filename);
  }
  
  /**
   * Function that trains the RNN on JSON trees
   * of dependency parse
   */
  bool TrainRnnModel();
  
  /**
   * Function that tests the RNN on JSON trees
   * of dependency parse
   */
  double TestRnnModel(const std::string &testFile,
                      const std::string &featureFile,
                      int &wordCounter,
                      std::vector<double> &sentenceScores);
  
protected:
  
  /**
   * Corpora
   */
  CorpusUnrolls m_corpusVocabulary;
  CorpusUnrolls m_corpusTrain;
  CorpusUnrolls m_corpusValidTest;
  
  /**
   * Label vocabulary representation (label -> index of the label)
   */
  int m_typeOfDepLabels;
  
  /**
   * Label vocabulary representation (label -> index of the label)
   */
  std::unordered_map<std::string, int> m_mapLabel2Index;
  
  /**
   * Return the index of a label in the label vocabulary, or -1 if OOV.
   */
  int SearchLabelInVocabulary(const std::string& word) const;
  
  /**
   * Reset the vector of feature labels
   */
  void ResetFeatureLabelVector(RnnState &state) const;
  
  /**
   * Update the vector of feature labels
   */
  void UpdateFeatureLabelVector(int label, RnnState &state) const;
};

#endif /* defined(__DependencyTreeRNN____RnnDependencyTreeLib__) */