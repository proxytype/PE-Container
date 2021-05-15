# PEContainer
![PECombiner](/pecombiner.PNG)

use PE features for store files in sections and export them using extractor method.

## PECombiner
create new sections in the extractor header and write file for each section, saved new file with all the data to be executed.

## PEExtractor
extract files from custom sections using the index section at the end of the sections list (.inx), 
the index section contains the filenames inside the payload.
