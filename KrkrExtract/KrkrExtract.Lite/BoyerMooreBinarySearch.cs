using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace KrkrExtract.Lite
{
    /// <summary>
    /// Uses the Boyer-Moore algorithm to perform binary searches.
    /// </summary>
    /// <remarks>
    /// The core implementation of the Boyer-Moore algorithm used here was taken from an article on the Code Project website
    /// located here: http://www.codeproject.com/Articles/12781/Boyer-Moore-and-related-exact-string-matching-algo
    /// and is covered by the Code Project Open License (CPOL) 1.02 located here: http://www.codeproject.com/info/cpol10.aspx
    /// </remarks>
    public class BoyerMooreBinarySearch
    {
        #region Members
        private readonly long[] _badCharacterShift;
        private readonly long[] _goodSuffixShift;
        private readonly long[] _suffixes;
        private readonly byte[] _searchPattern;
        #endregion Members

        #region Constructor

        /// <summary>
        /// Constructor
        /// </summary>
        /// <param name="searchPattern">Pattern for search</param>
        public BoyerMooreBinarySearch(byte[] searchPattern)
        {
            if ((searchPattern == null) ||
                !searchPattern.Any())
            {
                throw new ArgumentNullException("searchPattern");
            }

            /* Preprocessing */
            _searchPattern = searchPattern;
            _badCharacterShift = BuildBadCharacterShift(searchPattern);
            _suffixes = FindSuffixes(searchPattern);
            _goodSuffixShift = BuildGoodSuffixShift(searchPattern, _suffixes);
        }

        #endregion Constructor

        #region Public Methods

        /// <summary>
        /// Return all matches of the pattern in specified data using the Boyer-Moore algorithm
        /// </summary>
        /// <param name="dataToSearch">The data to be searched</param>
        /// <returns>IEnumerable which returns the indexes of pattern matches</returns>
        public ReadOnlyCollection<long> GetMatchIndexes(byte[] dataToSearch)
        {
            return new ReadOnlyCollection<long>(GetMatchIndexes_Internal(dataToSearch));
        }

        /// <summary>
        /// Return all matches of the pattern in specified file using the Boyer-Moore algorithm
        /// </summary>
        /// <param name="fileToSearch">The file to be searched</param>
        /// <param name="bufferSize">The size of the buffer to use when reading the file</param>
        /// <returns>IEnumerable which returns the indexes of pattern matches</returns>
        public ReadOnlyCollection<long> GetMatchIndexes(FileInfo fileToSearch, int bufferSize = 1024 * 1024)
        {
            var matchIndexes = new List<long>();

            if (bufferSize <= 0)
            {
                throw new ArgumentOutOfRangeException("bufferSize", bufferSize, @"Size of the file buffer must be greater than zero.");
            }
            int maxBufferSizeAllowed = (Int32.MaxValue - (_searchPattern.Length - 1));
            if (bufferSize > maxBufferSizeAllowed)
            {
                throw new ArgumentOutOfRangeException("bufferSize", bufferSize, String.Format("Size of the file buffer ({0}) plus the size of the search pattern minus one ({1}) may not exceed Int32.MaxValue ({2}).", bufferSize, (_searchPattern.Length - 1), Int32.MaxValue));
            }

            if ((fileToSearch != null) &&
                fileToSearch.Exists)
            {
                using (FileStream stream = fileToSearch.OpenRead())
                {
                    // Make sure that the file stream is seekable since that's how we'll be accessing the data
                    if (!stream.CanSeek)
                    {
                        throw new Exception(String.Format("The file '{0}' is not seekable!  Search cannot be performed.", fileToSearch));
                    }

                    int chunkIndex = 0;
                    while (true)
                    {
                        byte[] fileData = GetNextChunkForSearch(stream, chunkIndex, bufferSize);
                        if ((fileData != null) &&
                            fileData.Any())
                        {
                            List<long> occuranceIndexes = GetMatchIndexes_Internal(fileData);
                            if (occuranceIndexes != null)
                            {
                                // We found one or more matches in our buffer.  Translate the buffer index
                                // back to the file index by adding the buffer size * the chunk index.
                                int bufferOffset = (bufferSize * chunkIndex);
                                matchIndexes.AddRange(occuranceIndexes.Select(bufferMatchIndex => (bufferMatchIndex + bufferOffset)));
                            }
                        }
                        else
                        {
                            // We have reached the end of the file, so exit
                            break;
                        }
                        chunkIndex++;
                    } // end while
                }
            }

            return new ReadOnlyCollection<long>(matchIndexes);
        }

        #endregion Public Methods

        #region Helpers

        /// <summary>
        /// Build the bad byte shift array.
        /// </summary>
        /// <param name="pattern">Pattern for search</param>
        /// <returns>Bad byte shift array</returns>
        private long[] BuildBadCharacterShift(byte[] pattern)
        {
            var badCharacterShift = new long[256];
            long patternLength = Convert.ToInt64(pattern.Length);

            for (long c = 0; c < Convert.ToInt64(badCharacterShift.Length); ++c)
            {
                badCharacterShift[c] = patternLength;
            }

            for (long i = 0; i < patternLength - 1; ++i)
            {
                badCharacterShift[pattern[i]] = patternLength - i - 1;
            }

            return badCharacterShift;
        }

        /// <summary>
        /// Find suffixes in the pattern
        /// </summary>
        /// <param name="pattern">Pattern for search</param>
        /// <returns>Suffix array</returns>
        private long[] FindSuffixes(byte[] pattern)
        {
            long f = 0;

            var patternLength = Convert.ToInt64(pattern.Length);
            var suffixes = new long[pattern.Length + 1];

            suffixes[patternLength - 1] = patternLength;
            long g = patternLength - 1;
            for (long i = patternLength - 2; i >= 0; --i)
            {
                if (i > g && suffixes[i + patternLength - 1 - f] < i - g)
                {
                    suffixes[i] = suffixes[i + patternLength - 1 - f];
                }
                else
                {
                    if (i < g)
                    {
                        g = i;
                    }
                    f = i;
                    while (g >= 0 && (pattern[g] == pattern[g + patternLength - 1 - f]))
                    {
                        --g;
                    }
                    suffixes[i] = f - g;
                }
            }

            return suffixes;
        }

        /// <summary>
        /// Build the good suffix array.
        /// </summary>
        /// <param name="pattern">Pattern for search</param>
        /// <param name="suff">Suffixes in the pattern</param>
        /// <returns>Good suffix shift array</returns>
        private long[] BuildGoodSuffixShift(byte[] pattern, long[] suff)
        {
            var patternLength = Convert.ToInt64(pattern.Length);
            var goodSuffixShift = new long[pattern.Length + 1];

            for (long i = 0; i < patternLength; ++i)
            {
                goodSuffixShift[i] = patternLength;
            }

            long j = 0;
            for (long i = patternLength - 1; i >= -1; --i)
            {
                if (i == -1 || suff[i] == i + 1)
                {
                    for (; j < patternLength - 1 - i; ++j)
                    {
                        if (goodSuffixShift[j] == patternLength)
                        {
                            goodSuffixShift[j] = patternLength - 1 - i;
                        }
                    }
                }
            }

            for (long i = 0; i <= patternLength - 2; ++i)
            {
                goodSuffixShift[patternLength - 1 - suff[i]] = patternLength - 1 - i;
            }

            return goodSuffixShift;
        }

        /// <summary>
        /// Gets the next chunk of bytes from the stream to perform the search.
        /// </summary>
        /// <param name="stream">The stream containing the file to search</param>
        /// <param name="chunkIndex">The index of the chunk to read from the file</param>
        /// <param name="fileSearchBufferSize">The size of the data chunk to read from the file</param>
        /// <returns>The bytes read out of the stream</returns>
        private byte[] GetNextChunkForSearch(Stream stream, int chunkIndex, int fileSearchBufferSize)
        {
            byte[] chunk = null;

            long fileStartIndex = Convert.ToInt64(chunkIndex) * Convert.ToInt64(fileSearchBufferSize);
            if (fileStartIndex < stream.Length)
            {
                // Go to the start index where we want to begin reading from the file
                stream.Seek(fileStartIndex, SeekOrigin.Begin);

                // Get the number of bytes in the search pattern
                int searchBytesLength = _searchPattern.Length;

                // We will read the length of the buffer plus the length of the search array minus one.  This will allow us to find
                // instances where the search array crosses the boundary between two buffers.
                int bufferSize = fileSearchBufferSize + (searchBytesLength - 1);
                var buffer = new byte[bufferSize];
                long numBytesRead = Convert.ToInt64(stream.Read(buffer, 0, bufferSize));

                // If we read more bytes than the lenght of the search string, then return the bytes read.
                // If we read less than the search string length, then there's no point in returning them as a match can't be found.
                if (numBytesRead >= searchBytesLength)
                {
                    // If we read less than the buffer length (end of file), then return a trimmed byte array.
                    if (numBytesRead < bufferSize)
                    {
                        chunk = new byte[numBytesRead];
                        Array.Copy(buffer, chunk, numBytesRead);
                    }
                    else
                    {
                        // We filled the buffer entirely, so return it.
                        chunk = buffer;
                    }
                }

            }

            return chunk;
        }

        /// <summary>
        /// Return all matches of the pattern in specified data using the Boyer-Moore algorithm
        /// </summary>
        /// <param name="dataToSearch">The data to be searched</param>
        /// <returns>List which returns the indexes of pattern matches</returns>
        private List<long> GetMatchIndexes_Internal(byte[] dataToSearch)
        {
            var matchIndexes = new List<long>();

            if (dataToSearch == null)
            {
                throw new ArgumentNullException("dataToSearch");
            }

            long patternLength = Convert.ToInt64(_searchPattern.Length);
            long textLength = Convert.ToInt64(dataToSearch.Length);

            /* Searching */
            long index = 0;
            while (index <= (textLength - patternLength))
            {
                long unmatched;
                for (unmatched = patternLength - 1; unmatched >= 0 && (_searchPattern[unmatched] == dataToSearch[unmatched + index]); --unmatched)
                {
                    // empty - do nothing
                }

                if (unmatched < 0)
                {
                    matchIndexes.Add(index);
                    index += _goodSuffixShift[0];
                }
                else
                {
                    index += Math.Max(_goodSuffixShift[unmatched], _badCharacterShift[dataToSearch[unmatched + index]] - patternLength + 1 + unmatched);
                }
            }

            return matchIndexes;
        }
        #endregion Helpers
    }
}
