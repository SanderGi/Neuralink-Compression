# Neuralink Compression
The goal is 200x compression of 5 second uncompressed mono-channel wave files in real time (< 1ms) at low power (< 10mW).

## Current Solution (2.49x compression)
I noticed that most 5 second clips are sparse and only use a few hundred values from their range (-32768, 32767). This lends itself very well to simple Huffman encoding (which is nice and fast as well). However at the greater ends of the range (512-1019 unique amplitude values) I found it more efficient to simply store an array of unique values and a sequence of indices into that array. This hybrid approach is the current solution.

## Future Improvements
1. There are a few bytes to save by compressing the 44 byte wav header (or leaving it out entirely as it is the same for all files). I did not micro-optimize this for these preliminary experiments.
2. Arithmetic coding or asymmetric numeral systems are modern improvements over Huffman coding which trade some of the simplicity and speed for better compression ratios. These should bring the solution closer to the 200x goal.
3. More approaches could be incorporated into a hybrid solution. There are some clear classifications of files that have different characteristics (some are squarewaves, some are sinusoidal, some have more unique values than others) that can be exploited by different algorithms. I exploited the most common (very few unique values, and few unique values). More approaches could be incorporated to handle the other common cases.
4. Other facets outside the scopes of this mini challenge are also worth exploring, e.g., using a more optimal interval than 5 seconds (the more data, the more patterns, the greater the compression ratio), compressing multiple electrode streams together, etc. An important aspect to consider in the real time streaming case (not fixed 5 second intervals) is that the protocol can define incremental adjustments to the Huffman tree. That is, it does not need to send it. Both sender and reciever can start with the same huristically optimized tree and have an agreed algorithm to modify it based on the encoded/decoded data as they go along. This can save about 10% of the bandwidth on the Huffman tree alone and lead to more optimized compression of the data samples themselves. On test.wav, this brought the compression ratio from 2.7 to 3.0.

## Attempts that did not work
*Consult eda.ipynb for explorations of methods and dataviz*

1. Various lossles audio codecs (FLAC, OptimFROG, WavPack). These were decent but not as good as zip or the current solution and did not lend themselves to further compression.
2. Fast Fourier Transform (FFT) and variants (Discrete Cosine Transform, Wavelet Transforms, etc.) with quantization and amplitude thresholding followed by storing the delta to make them exact (lossless). These were too noisy to compress the deltas and lost too much information during quantization/thresholding to be useful.
3. Various forms of naive compression (run length encoding, delta encoding, etc.) were not effective on the data due to the lack of repeating patterns in most of the files.

## Build Instructions
*Tested on MacOS 14.4.1*

1. Clone the repository
2. Add `data.zip` to the root directory (zip file containing wave files)
3. Run `make` to build the project and/or `make eval` to run the evaluation script
