#pragma once

#include "data/Event.h"
#include "util/Util.h"

#include <unordered_set>


template <typename T>
/**
 * @brief Wrapper for different algorithms to calculate the similarity between two sequences.
 *
 * "a" is always the original sequences to compare against,
 *  and "b" is the generated sequence to be compared.
 */
class Similarity {
public:
	/**
	 * @brief Computes the fraction of notes that match exactly between two sequences.
	 *
	 * Useful as a simple baseline similarity metric.
	 */
	static double exactMatchSimilarity(const std::vector<T>& a, const std::vector<T>& b) {
		int matchCount = 0;
		const size_t len = std::min(a.size(), b.size());
		for (size_t i = 0; i < len; ++i) {
			if (a[i] == b[i]) matchCount++;
		}
		return static_cast<double>(matchCount) / len;
	}

	/**
	 * @brief Calculates the normalized Levenshtein similarity between two sequences.
	 *
	 * Reflects how many insertions, deletions, or substitutions are needed
	 * to transform one sequence into the other.
	 */
	static double levenshteinSimilarity(const std::vector<T>& a, const std::vector<T>& b) {
		const size_t m = a.size();
		const size_t n = b.size();
		std::vector dp(m + 1, std::vector<size_t>(n + 1));

		for (size_t i = 0; i <= m; ++i) dp[i][0] = i;
		for (size_t j = 0; j <= n; ++j) dp[0][j] = j;

		for (size_t i = 1; i <= m; ++i) {
			for (size_t j = 1; j <= n; ++j) {
				if (a[i - 1] == b[j - 1]) {
					dp[i][j] = dp[i - 1][j - 1];
				} else {
					dp[i][j] = std::min({ dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1] }) + 1;
				}
			}
		}

		const size_t distance = dp[m][n];
		const size_t maxLen = std::max(m, n);
		return maxLen > 0 ? 1.0 - static_cast<double>(distance) / maxLen : 1.0;
	}

	/**
	 * @brief Computes similarity based on shared n-grams between two sequences.
	 *
	 * Measures pattern overlap using the Jaccard index of n-sized sequences.
	 */
	static double ngramSimilarity(const std::vector<T>& a, const std::vector<T>& b, size_t n) {
		auto makeNGrams = [n](const std::vector<T>& seq) {
			std::unordered_set<std::string> ngrams;
			if (seq.size() < n)
				return ngrams;

			for (size_t i = 0; i <= seq.size() - n; ++i) {
				std::stringstream ss;
				for (size_t j = 0; j < n; ++j) {
					ss << static_cast<int>(seq[i + j]) << "-";
				}
				ngrams.insert(ss.str());
			}
			return ngrams;
		};

		const std::unordered_set<std::string> aGrams = makeNGrams(a);
		const std::unordered_set<std::string> bGrams = makeNGrams(b);

		// Calculate Jaccard index (intersection over union)
		size_t intersectionSize = 0;
		for (const auto& gram: aGrams) {
			if (bGrams.contains(gram)) intersectionSize++;
		}

		const size_t unionSize = aGrams.size() + bGrams.size() - intersectionSize;
		return unionSize > 0
			? static_cast<double>(intersectionSize) / static_cast<double>(unionSize)
			: 1.0;
	}
};


using SimilarityFunc = std::function<double(const Melody&, const Melody&)>;
inline std::vector<std::pair<std::string, SimilarityFunc>> similarityFunctions = {
	{
		"Exact Match",
		[](const Melody& a, const Melody& b) {
			return Similarity<Note>::exactMatchSimilarity(flattenNotes(a), flattenNotes(b));
		}
	},
	{
		"Levenshtein",
		[](const Melody& a, const Melody& b) {
			return Similarity<Note>::levenshteinSimilarity(flattenNotes(a), flattenNotes(b));
		}
	},
	{
		"3-gram",
		[](const Melody& a, const Melody& b) {
			return Similarity<Note>::ngramSimilarity(flattenNotes(a), flattenNotes(b), 3);
		}
	},
	{
		"4-gram",
		[](const Melody& a, const Melody& b) {
			return Similarity<Note>::ngramSimilarity(flattenNotes(a), flattenNotes(b), 4);
		}
	},
	{
		"5-gram",
		[](const Melody& a, const Melody& b) {
			return Similarity<Note>::ngramSimilarity(flattenNotes(a), flattenNotes(b), 5);
		}
	}
};
