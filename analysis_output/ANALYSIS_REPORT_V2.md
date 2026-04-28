# Sonar Analysis Report V2

## Quantitative comparison (lower smoothness = smoother)

| File | Frames | BOTH_ok | Events | Smooth Fallback | Median | EWMA | Kalman | Outliers FB | Med | EWA | KF |
|------|--------|---------|--------|-----------------|--------|------|--------|-------------|-----|-----|----|
| exp_001_walk_left_to_right_close.txt     |    150 |      39 |     22 |           127.9 |   26.5 | 20.1 |   48.5 |          24 |   0 |   0 |  0 |
| exp_002_walk_left_to_right_medium.txt    |    139 |      14 |     12 |           162.7 |  176.8 | 189.5 |   99.5 |          19 |   1 |   0 |  0 |
| exp_003_walk_left_to_right_far.txt       |    150 |       0 |      4 |            16.5 |      - |    - |    inf |           4 |   0 |   0 |  0 |
| exp_004_walk_left_right_left_medium.txt  |    209 |      36 |     16 |           107.0 |   60.0 | 55.3 |   97.3 |          24 |   1 |   0 |  0 |
| exp_005_walk_left_center_left_medium.txt |    212 |      90 |      9 |           150.7 |   59.7 | 58.5 |   36.1 |          20 |   0 |   0 |  0 |
| exp_006_walk_right_center_right_medium.t |    220 |     109 |     18 |           165.2 |   40.6 | 47.4 |   37.8 |          26 |   0 |   0 |  0 |
| exp_007_walk_right_to_left_medium_fast.t |     69 |       6 |      8 |            95.2 |   42.6 | 25.1 |  272.1 |           8 |   0 |   0 |  1 |
| exp_008_walk_right_to_left_medium_fast.t |     58 |       2 |      8 |           144.1 |    0.0 |  inf |  203.9 |           9 |   0 |   0 |  1 |
| exp_009_walk_right_to_left_medium_fast.t |     64 |       1 |      5 |            69.9 |    0.0 |  inf |   58.0 |           6 |   0 |   0 |  0 |
| exp_010_walk_left_to_right_medium_fast.t |     61 |       9 |      5 |            91.5 |   34.5 | 40.8 |  165.5 |           6 |   0 |   0 |  0 |
| exp_011_walk_left_to_right_medium_fast.t |     61 |       8 |      4 |            70.6 |   25.3 | 46.3 |  213.2 |           4 |   0 |   0 |  0 |
| exp_012_walk_left_to_right_medium_fast.t |     61 |       7 |      3 |            70.1 |   47.6 | 54.0 |  161.2 |           4 |   0 |   0 |  0 |
| exp_013_walk_left_center_left_far.txt    |    107 |       0 |      4 |            25.5 |      - |    - |    inf |           4 |   0 |   0 |  0 |
| exp_014_walk_left_center_left_far.txt    |    116 |       0 |      4 |            17.0 |      - |    - |    0.0 |           3 |   0 |   0 |  0 |
| exp_015_walk_left_center_left_far.txt    |    125 |       1 |      8 |            47.0 |    0.0 |  inf |  273.4 |           8 |   0 |   0 |  0 |
| exp_016_walk_left_center_left_far.txt    |    157 |       0 |      6 |            37.2 |      - |    - |  404.8 |           6 |   0 |   0 |  1 |
| exp_017_walk_center_close_far_close.txt  |    213 |     160 |     17 |          4692.6 | 2835.4 | 2011.2 | 4725.8 |          23 |   1 |   0 |  0 |
| exp_018_walk_center_close_far_close.txt  |    238 |     177 |     17 |          6237.9 | 6371.7 | 2363.1 | 1688.4 |          23 |   0 |   0 |  0 |
| exp_019_walk_center_close_far_close.txt  |    221 |     160 |     18 |          8960.3 |   36.4 | 3959.9 | 3027.5 |          25 |   0 |   0 |  0 |
| exp_020_walk_left_center_left_close.txt  |    152 |      63 |      6 |           141.2 |   42.3 | 52.7 |   38.7 |          14 |   0 |   0 |  0 |
| exp_021_walk_left_center_left_close.txt  |    163 |      88 |      8 |           155.4 |   64.3 | 58.6 |   38.7 |          17 |   1 |   0 |  0 |
| exp_022_walk_left_center_left_close.txt  |    150 |      78 |      7 |           104.9 |   25.4 | 36.2 |   33.1 |           8 |   0 |   0 |  0 |
| exp_023_walk_center_close_medium_far.txt |    173 |     133 |     11 |           142.2 |   30.3 | 25.1 |   13.8 |          17 |   0 |   0 |  0 |
| exp_024_walk_left_close_medium_far.txt   |    138 |       8 |     18 |            97.0 |   25.9 | 16.4 |   15.7 |          16 |   0 |   0 |  0 |
| exp_025_walk_left_close_medium_far.txt   |    132 |       0 |     10 |            47.3 |      - |    - |    0.0 |           9 |   0 |   0 |  0 |
