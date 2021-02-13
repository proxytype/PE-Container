    ____  ______   ______            __        _                
   / __ \/ ____/  / ____/___  ____  / /_____ _(_)___  ___  _____
  / /_/ / __/    / /   / __ \/ __ \/ __/ __ `/ / __ \/ _ \/ ___/
 / ____/ /___   / /___/ /_/ / / / / /_/ /_/ / / / / /  __/ /    
/_/   /_____/   \____/\____/_/ /_/\__/\__,_/_/_/ /_/\___/_/     
                                                                

# PEContainer
use PE features for store files in sections and export them using extractor methof

## PECombiner
create new sections in the extractor header and write file for each section, saved new file with all the data to be executed.

## PEExtractor
this extractor extract files from custom sections using index section at the end of the sections list (.inx), 
the index section contains the filenames combine inside the extractor.
