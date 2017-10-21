
Get the results:
* Create separate dirs for each configuration with a symlink on run_p2p_set.sh
* copy env_pattern.sh to each of them (`mv env_pattern.sh env.sh`) and customize it
* Run either using:
  - `./run_p2p_set.sh -pml="{ucx|mxm}/mlx5_x:p/{rc|dc|rc_x|dc_x}" {-dir="<osu-pt2pt-dir>|-bin="osu-binary"} [-hcol="{ucx|mxm}/{rc|dc|rc_x|dc_x}"] [-large] [-iter=<iter-num-if-multi>]`
    see other options in `run_p2p_set.sh`
  - customize run_set_of.sh which usually runs in the topdir relative to config directories.

Postprocess:
* Use `postprocess_all.sh <config-dir> <result-dir> <suffix>`, <suffix> will be appended to the filenames.

Compare:
* Use `compare.sh <result-dir1> <result-dir2> > output.cmp`
