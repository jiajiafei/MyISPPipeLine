#include"demosaic.h"
#include<iostream>

#define extern_num 2
inline int idx(int x, int y, int W) { return y * W + x; };
u16 gW_image = 0;
u16 ReturnPixValueExtened(u16* A, int i, int j)
{

    return A[(i) * (gW_image + 2 * extern_num) + (j)];
}
void fRaw_extern(u16* raw_original, u16* raw_externed, u16 H_image,u16 W_image)
{
    memset(raw_externed, 1, (H_image + 2 * extern_num) * (W_image + 2 * extern_num));
    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            raw_externed[(i + extern_num) * (2 * extern_num + W_image) + j + extern_num] = raw_original[i * W_image + j];
        }
    }
    //top
    for (int i = 0; i < extern_num; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            raw_externed[(i) * (2 * extern_num + W_image) + j + extern_num] = raw_original[i * W_image + j];
        }
    }
    //down
    for (int i = H_image - extern_num; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            raw_externed[(i + 2 * extern_num) * (2 * extern_num + W_image) + j + extern_num] = raw_original[i * W_image + j];
        }
    }
    //left
    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < extern_num; j++)
        {
            raw_externed[(i + extern_num) * (2 * extern_num + W_image) + j] = raw_original[i * W_image + j];
        }
    }
    //right
    for (int i = 0; i < H_image; i++)
    {
        for (int j = W_image - extern_num; j < W_image; j++)
        {
            raw_externed[(i + extern_num) * (2 * extern_num + W_image) + j + 2 * extern_num] = raw_original[i * W_image + j];
        }
    }
};
#if 0
void fOri_Dir(unsigned short* raw_externed, unsigned short* Dir_map, stISPParams* gISPparam)//RGGB
{
    int H_image = gISPparam->rawinfo.H;
    int W_image = gISPparam->rawinfo.W;
    int H_new = H_image + 2 * extern_num;
    int W_image_new = W_image + 2 * extern_num;
    unsigned short* H_dela = (unsigned short*)malloc((H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    unsigned short* V_dela = (unsigned short*)malloc((H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    memset(H_dela, 0, (H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    memset(V_dela, 0, (H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));


    for (int i = 2; i < H_new - 2; i++)
    {
        for (int j = 2; j < W_image_new - 2; j++)
        {
            H_dela[i * W_image_new + j] = abs(ReturnPixValueExtened(raw_externed, i, j - 1) - ReturnPixValueExtened(raw_externed, i, j + 1)) +
                abs(2 * ReturnPixValueExtened(raw_externed, i, j) - ReturnPixValueExtened(raw_externed, i, j - 2) - ReturnPixValueExtened(raw_externed, i, j + 2));

            V_dela[i * W_image_new + j] = abs(ReturnPixValueExtened(raw_externed, i - 1, j) - ReturnPixValueExtened(raw_externed, i + 1, j)) +
                abs(2 * ReturnPixValueExtened(raw_externed, i, j) - ReturnPixValueExtened(raw_externed, i - 2, j) - ReturnPixValueExtened(raw_externed, i + 2, j));

        }

    }
    unsigned short* Hi_dela = (unsigned short*)malloc((H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    unsigned short* Vi_dela = (unsigned short*)malloc((H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    memset(Hi_dela, 0, (H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    memset(Vi_dela, 0, (H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(unsigned short)));
    for (int i = 2; i < H_new - 2; i++)
    {
        for (int j = 2; j < W_image_new - 2; j++)
        {
            if (((i % 2 == 0) && (j % 2 == 0)) || (i % 2 == 1) && (j % 2 == 1))
            {
                Hi_dela[i * W_image_new + j] = 0.5 * H_dela[i * W_image_new + j] + 0.25 * (H_dela[(i - 1) * W_image_new + j] + H_dela[(i + 1) * W_image_new + j]) + 0.125 * (H_dela[(i - 2) * W_image_new + j] + H_dela[(i + 2) * W_image_new + j]);
                Vi_dela[i * W_image_new + j] = 0.5 * V_dela[i * W_image_new + j] + 0.25 * (V_dela[(i)*W_image_new + j + 1] + H_dela[(i)*W_image_new + j - 1]) + 0.125 * (H_dela[(i)*W_image_new + j - 2] + H_dela[(i)*W_image_new + j + 2]);
                // Hi_dela[i *W_image_new + j] = H_dela[i *W_image_new + j];
                 //Vi_dela[i *W_image_new + j] =  V_dela[i *W_image_new + j];


                int G_ave = 0.25 * (ReturnPixValueExtened(raw_externed, i, j - 1) + ReturnPixValueExtened(raw_externed, i, j + 1) + ReturnPixValueExtened(raw_externed, i - 1, j) + ReturnPixValueExtened(raw_externed, i + 1, j));
                int R_ave = 0;
                int B_ave = 0;
                int HD_r = 0;
                int HD_b = 0;
                int VD_r = 0;
                int VD_b = 0;

                if (i % 2 == 0)
                {
                    R_ave = ReturnPixValueExtened(raw_externed, i, j);
                    B_ave = 0.25 * (ReturnPixValueExtened(raw_externed, i - 1, j - 1) + ReturnPixValueExtened(raw_externed, i - 1, j + 1) + ReturnPixValueExtened(raw_externed, i + 1, j + 1) + ReturnPixValueExtened(raw_externed, i + 1, j - 1));
                    HD_r = abs(ReturnPixValueExtened(raw_externed, i, j - 1) + ReturnPixValueExtened(raw_externed, i, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i, j));
                    VD_r = abs(ReturnPixValueExtened(raw_externed, i - 1, j) + ReturnPixValueExtened(raw_externed, i + 1, j) - 2 * ReturnPixValueExtened(raw_externed, i, j));
                    HD_b = 0.5 * (abs(ReturnPixValueExtened(raw_externed, i - 1, j - 1) + ReturnPixValueExtened(raw_externed, i - 1, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i - 1, j))
                        + abs(ReturnPixValueExtened(raw_externed, i + 1, j - 1) + ReturnPixValueExtened(raw_externed, i + 1, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i + 1, j)));
                    VD_b = 0.5 * (abs(ReturnPixValueExtened(raw_externed, i - 1, j - 1) + ReturnPixValueExtened(raw_externed, i + 1, j - 1) - 2 * ReturnPixValueExtened(raw_externed, i, j - 1))
                        + abs(ReturnPixValueExtened(raw_externed, i - 1, j + 1) + ReturnPixValueExtened(raw_externed, i - 1, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i, j + 1)));

                }
                else
                {
                    B_ave = ReturnPixValueExtened(raw_externed, i, j);
                    R_ave = 0.25 * (ReturnPixValueExtened(raw_externed, i - 1, j - 1) + ReturnPixValueExtened(raw_externed, i - 1, j + 1) + ReturnPixValueExtened(raw_externed, i + 1, j + 1) + ReturnPixValueExtened(raw_externed, i + 1, j - 1));
                    HD_r = 0.5 * (abs(ReturnPixValueExtened(raw_externed, i - 1, j - 1) + ReturnPixValueExtened(raw_externed, i - 1, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i - 1, j))
                        + abs(ReturnPixValueExtened(raw_externed, i + 1, j - 1) + ReturnPixValueExtened(raw_externed, i + 1, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i + 1, j)));
                    VD_r = 0.5 * (abs(ReturnPixValueExtened(raw_externed, i - 1, j - 1) + ReturnPixValueExtened(raw_externed, i + 1, j - 1) - 2 * ReturnPixValueExtened(raw_externed, i, j - 1))
                        + abs(ReturnPixValueExtened(raw_externed, i - 1, j + 1) + ReturnPixValueExtened(raw_externed, i - 1, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i, j + 1)));
                    HD_b = abs(ReturnPixValueExtened(raw_externed, i, j - 1) + ReturnPixValueExtened(raw_externed, i, j + 1) - 2 * ReturnPixValueExtened(raw_externed, i, j));
                    VD_b = abs(ReturnPixValueExtened(raw_externed, i - 1, j) + ReturnPixValueExtened(raw_externed, i + 1, j) - 2 * ReturnPixValueExtened(raw_externed, i, j));
                }
                int C_GR = abs(G_ave - R_ave);
                int C_GB = abs(G_ave - B_ave);
                int H_dir = 0;
                int V_dir = 0;
                if (C_GR >= C_GB)
                {
                    H_dir = HD_r * gISPparam->demosaic_Param.m_alpha + Hi_dela[i * W_image_new + j];
                    V_dir = VD_r * gISPparam->demosaic_Param.m_alpha + Vi_dela[i * W_image_new + j];
                }
                else
                {
                    H_dir = HD_b * gISPparam->demosaic_Param.m_alpha + Hi_dela[i * W_image_new + j];
                    V_dir = VD_b * gISPparam->demosaic_Param.m_alpha + Vi_dela[i * W_image_new + j];
                }
                if (V_dir - H_dir >= gISPparam->demosaic_Param.m_HVgap)
                {
                    Dir_map[i * W_image_new + j] = 1;
                }
                else if (H_dir - V_dir >= gISPparam->demosaic_Param.m_HVgap)
                {
                    Dir_map[i * W_image_new + j] = 2;
                }
                else
                {
                    Dir_map[i * W_image_new + j] = 0;
                }
            }
            else
            {
                Hi_dela[i * W_image_new + j] = 0.5 * H_dela[i * W_image_new + j] + 0.25 * (H_dela[(i - 1) * W_image_new + j] + H_dela[(i + 1) * W_image_new + j]) + 0.125 * (H_dela[(i - 2) * W_image_new + j] + H_dela[(i + 2) * W_image_new + j]);
                Vi_dela[i * W_image_new + j] = 0.5 * V_dela[i * W_image_new + j] + 0.25 * (V_dela[(i)*W_image_new + j + 1] + H_dela[(i)*W_image_new + j - 1]) + 0.125 * (H_dela[(i)*W_image_new + j - 2] + H_dela[(i)*W_image_new + j + 2]);
                int H_dir = Hi_dela[i * W_image_new + j];
                int V_dir = Vi_dela[i * W_image_new + j];
                if (V_dir - H_dir >= gISPparam->demosaic_Param.m_HVgap)
                {
                    Dir_map[i * W_image_new + j] = 1;
                }
                else if (H_dir - V_dir >= gISPparam->demosaic_Param.m_HVgap)
                {
                    Dir_map[i * W_image_new + j] = 2;
                }
                else
                {
                    Dir_map[i * W_image_new + j] = 0;
                }
            }
        }

    }
    unsigned short* Dir_mapCpy = (unsigned short*)malloc(H_new * W_image_new * sizeof(unsigned short));
    memcpy(Dir_mapCpy, Dir_map, H_new * W_image_new * sizeof(unsigned short));


    for (int i = 2; i < H_new - 2; i++)
    {
        for (int j = 2; j < W_image_new - 2; j++)
        {
            if (Dir_mapCpy[i * W_image_new + j] == 0)
            {
                //calculate directions
                int dirValue[3] = { 0 };
                for (int k_i = -1; k_i <= 1; k_i++)
                {
                    for (int k_j = -1; k_j <= 1; k_j++)
                    {
                        if (Dir_mapCpy[(i + k_i) * W_image_new + j + k_j] < 3) ++dirValue[Dir_mapCpy[(i + k_i) * W_image_new + j + k_j]];

                    }
                }
                if ((dirValue[0] + dirValue[1] + dirValue[2] == 9) && (dirValue[0] && dirValue[1] && dirValue[2] == 0))//除了中心点，仅有两个方向
                {
                    int maxValue = 0;
                    int maxIndex = 0;
                    for (int v_i = 0; v_i < 3; v_i++)
                    {
                        if (maxValue < dirValue[v_i])
                        {
                            maxValue = dirValue[v_i];
                            maxIndex = v_i;
                        }
                    }
                    if (maxValue >= 6)
                    {
                        Dir_map[i * W_image_new + j] = maxIndex;
                    }

                }
            }
        }
    }

    //middle filter

    /**/
    memset(Dir_mapCpy, 0, sizeof(unsigned short) * W_image_new * H_new);
    memcpy(Dir_mapCpy, Dir_map, sizeof(unsigned short) * W_image_new * H_new);

    for (int i = 2; i < H_new - 2; i++)
    {
        for (int j = 2; j < W_image_new - 2; j++)
        {
            if (((i % 2 == 0) && (j % 2 == 0)) || (i % 2 == 1) && (j % 2 == 1))
            {
                int dirCount[3] = { 0 };
                for (int dir = 0; dir < 3; dir++)
                {
                    if (dir == Dir_mapCpy[i * W_image_new + j]) dirCount[dir]++;
                    if (dir == Dir_mapCpy[(i - 2) * W_image_new + j]) dirCount[dir]++;
                    if (dir == Dir_mapCpy[i * W_image_new + j - 2]) dirCount[dir]++;
                    if (dir == Dir_mapCpy[i * W_image_new + j + 2]) dirCount[dir]++;
                    if (dir == Dir_mapCpy[(i + 2) * W_image_new + j]) dirCount[dir]++;
                }
                int maxdir = 0;
                int maxindex = 0;
                for (int dir = 0; dir < 3; dir++)
                {
                    if (dirCount[dir] > maxdir)
                    {
                        maxdir = dirCount[dir];
                        maxindex = dir;
                    }
                }
                if (maxdir > 4)
                {
                    Dir_map[W_image_new * i + j] = maxindex;
                }
            }

        }
    }

}
void interpolate(unsigned short* raw_externed, unsigned short* Dir_map, unsigned short* interpolated, stISPParams* gISPparam)
{
    //g
    int H_image = gISPparam->rawinfo.H;
    int W_image = gISPparam->rawinfo.W;
    int H_new = H_image + 2 * extern_num;
    int W_image_new = W_image + 2 * extern_num;
    int MaxValue = (1<<gISPparam->demosaic_Param.Demosaicbit)-1;
    for (int i = 2; i < H_new - 2; i++)
    {
        for (int j = 2; j < W_image_new - 2; j++)
        {
            if (((i % 2 == 0) && (j % 2 == 0)) || ((i % 2 == 1) && (j % 2 == 1)))
            {
#if 1
                if (Dir_map[i * W_image_new + j] == 1)//h
                {

                    interpolated[(i - extern_num) * W_image + j - 2] = CLIP((raw_externed[i * W_image_new + j - 1] + raw_externed[i * W_image_new + j + 1]) / 2
                        + (2 * raw_externed[i * W_image_new + j] - raw_externed[i * W_image_new + j + 2] - raw_externed[i * W_image_new + j - 2]) / 4, 0, MaxValue);

                }
                else if (Dir_map[i * W_image_new + j] == 2)
                {

                    interpolated[(i - extern_num) * W_image + j - 2] = CLIP((raw_externed[(i - 1) * W_image_new + j] + raw_externed[(i + 1) * W_image_new + j]) / 2
                        + (2 * raw_externed[i * W_image_new + j] - raw_externed[(i + 2) * W_image_new + j] - raw_externed[(i - 2) * W_image_new + j]) / 4, 0, MaxValue);

                }
                else if (Dir_map[i * W_image_new + j] == 0)
                {

                    interpolated[(i - extern_num) * W_image + j - 2] = CLIP((raw_externed[i * W_image_new + j - 1] + raw_externed[i * W_image_new + j + 1] +
                        raw_externed[(i - 1) * W_image_new + j] + raw_externed[(i + 1) * W_image_new + j]) / 4
                        + (4 * raw_externed[i * W_image_new + j] - raw_externed[i * W_image_new + j - 2] - raw_externed[i * W_image_new + j + 2] -
                            raw_externed[(i - 2) * W_image_new + j] - raw_externed[(i + 2) * W_image_new + j]) / 8, 0, MaxValue);

                }
#endif
            }
            else
            {
                interpolated[(i - extern_num) * W_image + j - 2] = raw_externed[i * W_image_new + j];
            }
        }
    }

#if 1
    //b
    for (int i = 2; i < H_new - 2; i++)
    {
        for (int j = 2; j < W_image_new - 2; j++)
        {
            if (i - extern_num - 1 <= 0) i = extern_num + 1;
            //g1
            if ((i % 2 == 0) && (j % 2 == 1))
            {
                interpolated[(i - extern_num) * W_image + j - extern_num + H_image * W_image] = CLIP(0.5 * (raw_externed[(i - 1) * W_image_new + j] + raw_externed[(i + 1) * W_image_new + j]) +

                    0.5 * (2 * raw_externed[(i)*W_image_new + j] - interpolated[(i - extern_num - 1) * W_image + j - extern_num] - interpolated[(i - extern_num + 1) * W_image + j - extern_num]), 0, MaxValue);
            }
            //g2
            else if ((i % 2 == 1) && (j % 2 == 0))
            {
                interpolated[(i - extern_num) * W_image + j - extern_num + H_image * W_image] = CLIP(0.5 * (raw_externed[(i)*W_image_new + j - 1] + raw_externed[(i)*W_image_new + j + 1])
                    + 0.5 * (2 * raw_externed[(i)*W_image_new + j] - interpolated[(i - extern_num) * W_image + j - extern_num - 1] - interpolated[(i - extern_num) * W_image + j - extern_num + 1]), 0, MaxValue);
            }
            //r
            else if ((i % 2 == 0) && (j % 2 == 0))
            {
                int D45 = abs(raw_externed[(i - 1) * W_image_new + j + 1] - raw_externed[(i + 1) * W_image_new + j - 1])
                    + abs(2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num - 1]);
                int D135 = abs(raw_externed[(i - 1) * W_image_new + j - 1] - raw_externed[(i + 1) * W_image_new + j + 1])
                    + abs(2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num - 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num + 1]);


                if (D135 - D45 >= gISPparam->demosaic_Param.m_HVgap)
                {
                    interpolated[(i - extern_num) * W_image + j - extern_num + H_image * W_image] = 0.5 * (raw_externed[(i - 1) * W_image_new + j + 1] + raw_externed[(i + 1) * W_image_new + j - 1]) +
                        0.5 * (2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num - 1]);
                }

                else if (D45 - D135 >= gISPparam->demosaic_Param.m_HVgap)
                {
                    interpolated[(i - extern_num) * W_image + j - extern_num + H_image * W_image] = CLIP(0.5 * (raw_externed[(i - 1) * W_image_new + j - 1] + raw_externed[(i + 1) * W_image_new + j + 1])
                        + 0.5 * (2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num - 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num + 1]), 0, MaxValue);

                }

                else
                {
                    interpolated[(i - extern_num) * W_image + j - extern_num + H_image * W_image] = CLIP(0.25 * (raw_externed[(i - 1) * W_image_new + j - 1] + raw_externed[(i - 1) * W_image_new + j + 1] +
                        raw_externed[(i + 1) * W_image_new + j + 1] + raw_externed[(i + 1) * W_image_new + j - 1]) + 0.25 * (4 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num - 1]
                            - interpolated[(i - extern_num + 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num - 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num - 1]), 0, MaxValue);
                }

            }
            else
            {
                interpolated[(i - extern_num) * W_image + j - extern_num + H_image * W_image] = raw_externed[i * W_image_new + j];
            }
        }
    }


    //r
    for (int i = 3; i < H_new - 2; i++)
    {
        for (int j = 3; j < W_image_new - 2; j++)
        {
            //g1
            if ((i % 2 == 0) && (j % 2 == 1))
            {
                interpolated[(i - extern_num) * W_image + j - extern_num + 2 * H_image * W_image] = CLIP(0.5 * (raw_externed[(i)*W_image_new + j - 1] + raw_externed[(i)*W_image_new + j + 1])
                    + 0.5 * (2 * raw_externed[(i)*W_image_new + j] - interpolated[(i - extern_num) * W_image + j - extern_num - 1] - interpolated[(i - extern_num) * W_image + j - extern_num + 1]), 0, MaxValue);
            }
            //g2
            else if ((i % 2 == 1) && (j % 2 == 0))
            {
                interpolated[(i - extern_num) * W_image + j - extern_num + 2 * H_image * W_image] = CLIP(0.5 * (raw_externed[(i - 1) * W_image_new + j] + raw_externed[(i + 1) * W_image_new + j]) +
                    0.5 * (2 * raw_externed[(i)*W_image_new + j] - interpolated[(i - extern_num - 1) * W_image + j - extern_num] - interpolated[(i - extern_num + 1) * W_image + j - extern_num]), 0, MaxValue);
            }
            //b
            else if ((i % 2 == 1) && (j % 2 == 1))
            {
                int D45 = abs(raw_externed[(i - 1) * W_image_new + j + 1] - raw_externed[(i + 1) * W_image_new + j - 1])
                    + abs(2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num - 1]);
                int D135 = abs(raw_externed[(i - 1) * W_image_new + j - 1] - raw_externed[(i + 1) * W_image_new + j + 1])
                    + abs(2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num - 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num + 1]);


                if (D135 - D45 >= gISPparam->demosaic_Param.m_HVgap)
                {
                    interpolated[(i - extern_num) * W_image + j - extern_num + 2 * H_image * W_image] = CLIP(0.5 * (raw_externed[(i - 1) * W_image_new + j + 1] + raw_externed[(i + 1) * W_image_new + j - 1]) +
                        0.5 * (2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num - 1]), 0, MaxValue);
                }

                else if (D45 - D135 >= gISPparam->demosaic_Param.m_HVgap)
                {
                    interpolated[(i - extern_num) * W_image + j - extern_num + 2 * H_image * W_image] = CLIP(0.5 * (raw_externed[(i - 1) * W_image_new + j - 1] + raw_externed[(i + 1) * W_image_new + j + 1])
                        + 0.5 * (2 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num - 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num + 1]), 0, MaxValue);

                }

                else
                {
                    interpolated[(i - extern_num) * W_image + j - extern_num + 2 * H_image * W_image] = CLIP(0.25 * (raw_externed[(i - 1) * W_image_new + j - 1] + raw_externed[(i - 1) * W_image_new + j + 1] +
                        raw_externed[(i + 1) * W_image_new + j + 1] + raw_externed[(i + 1) * W_image_new + j - 1]) + 0.25 * (4 * interpolated[(i - extern_num) * W_image + j - extern_num] - interpolated[(i - extern_num - 1) * W_image + j - extern_num - 1]
                            - interpolated[(i - extern_num + 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num - 1) * W_image + j - extern_num + 1] - interpolated[(i - extern_num + 1) * W_image + j - extern_num - 1]), 0, MaxValue);
                }

            }
            else
            {
                interpolated[(i - extern_num) * W_image + j - extern_num + 2 * H_image * W_image] = raw_externed[i * W_image_new + j];
            }
        }
    }
#endif
}
#endif
#if 0
//原始的插值方法
int Interpolation(int H_image, int W_image, u16* u16raw_original, u16* mosaicResult, stISPParams* gISPparam)
{
    int imagesize = H_image * W_image;
    int* raw_original = (int*)calloc(imagesize, sizeof(int));
    int extrasize = 2;
    int index_cur = 0;
    int Bitmax = (1 << gISPparam->demosaic_Param.Demosaicbit) - 1;
    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            int ypos = i % 2;
            int xpos = j % 2;
            index_cur = idx(j, i, W_image);
            if ((ypos == 0) && (xpos == 0))//r
            {
                mosaicResult[index_cur] = u16raw_original[index_cur];
            }
            else if ((ypos == 1) && (xpos == 1))//b
            {
                mosaicResult[index_cur + 2 * imagesize] = u16raw_original[index_cur];
            }
            else
            {
                mosaicResult[index_cur + imagesize] = u16raw_original[index_cur];
            }

        }
    }
    for (int i = 0; i < imagesize; i++)
    {
        raw_original[i] = (int)u16raw_original[i];
    }
    int index_rig = 0;

    int index_lef = 0;
    int index_rig2 = 0;
    int index_lef2 = 0;

    int index_up = 0;
    int index_down = 0;
    int index_up2 = 0;
    int index_down2 = 0;
    float filter1[5] = { -0.25,0.5,0.5,0.5,-0.25 };
    int* GRdeltaV = (int*)calloc(imagesize, sizeof(int));
    int* GRdeltaH = (int*)calloc(imagesize, sizeof(int));

    int* GBdeltaV = (int*)calloc(imagesize, sizeof(int));
    int* GBdeltaH = (int*)calloc(imagesize, sizeof(int));

    int* deltaV = (int*)calloc(imagesize, sizeof(int));
    int* deltaH = (int*)calloc(imagesize, sizeof(int));

    int* deltaGR = (int*)calloc(imagesize, sizeof(int));
    int* deltaGB = (int*)calloc(imagesize, sizeof(int));

    int* DV = (int*)calloc(imagesize, sizeof(int));
    int* DH = (int*)calloc(imagesize, sizeof(int));

    float* Wn = (float*)calloc(imagesize, sizeof(float));
    float* Ws = (float*)calloc(imagesize, sizeof(float));
    float* Ww = (float*)calloc(imagesize, sizeof(float));
    float* We = (float*)calloc(imagesize, sizeof(float));

    for (int i = extrasize; i < H_image - extrasize; i++)
    {
        for (int j = extrasize; j < W_image - extrasize; j++)
        {
            index_rig = idx(j + 1, i, W_image);
            index_cur = idx(j, i, W_image);
            index_lef = idx(j - 1, i, W_image);
            index_rig2 = idx(j + 2, i, W_image);
            index_lef2 = idx(j - 2, i, W_image);
            index_up = idx(j, i - 1, W_image);
            index_down = idx(j, i + 1, W_image);
            index_up2 = idx(j, i - 2, W_image);
            index_down2 = idx(j, i + 2, W_image);

            int H = raw_original[index_lef2] * filter1[0] + raw_original[index_lef] * filter1[1] + raw_original[index_cur] * filter1[2] + \
                raw_original[index_rig] * filter1[3] + raw_original[index_rig2] * filter1[4];
            int V = raw_original[index_up2] * filter1[0] + raw_original[index_up] * filter1[1] + raw_original[index_cur] * filter1[2] + \
                raw_original[index_down] * filter1[3] + raw_original[index_down2] * filter1[4];;
            int ypos = i % 2;
            int xpos = j % 2;
            if ((ypos == 0) && (xpos == 0))//r
            {
                GRdeltaH[index_cur] = H - raw_original[index_cur];
                GRdeltaV[index_cur] = V - raw_original[index_cur];
                deltaV[index_cur] = V - raw_original[index_cur];
                deltaH[index_cur] = H - raw_original[index_cur];
            }
            else if ((ypos == 0) && (xpos == 1))//gr
            {
                GRdeltaH[index_cur] = raw_original[index_cur] - H;
                GBdeltaV[index_cur] = raw_original[index_cur] - V;
                deltaH[index_cur] = raw_original[index_cur] - H;
                deltaV[index_cur] = raw_original[index_cur] - V;
            }
            else if ((ypos == 1) && (xpos == 0))//gb
            {
                GBdeltaH[index_cur] = raw_original[index_cur] - H;
                GRdeltaV[index_cur] = raw_original[index_cur] - V;
                deltaH[index_cur] = raw_original[index_cur] - H;
                deltaV[index_cur] = raw_original[index_cur] - V;
            }
            else if ((ypos == 1) && (xpos == 1))//b
            {
                GBdeltaH[index_cur] = H - raw_original[index_cur];
                GBdeltaV[index_cur] = V - raw_original[index_cur];
                deltaH[index_cur] = H - raw_original[index_cur];
                deltaV[index_cur] = V - raw_original[index_cur];
            }
        }
    }
    for (int i = 1; i < H_image - 1; i++)
    {
        for (int j = 1; j < W_image - 1; j++)
        {
            index_rig = idx(j + 1, i, W_image);
            index_cur = idx(j, i, W_image);
            index_lef = idx(j - 1, i, W_image);
            index_up = idx(j, i - 1, W_image);
            index_down = idx(j, i + 1, W_image);

            DH[index_cur] = abs(deltaH[index_lef] - deltaH[index_rig]);
            DV[index_cur] = abs(deltaV[index_up] - deltaV[index_down]);
        }
    }

    for (int i = 4; i < H_image; i++)
    {
        for (int j = 2; j < W_image - 2; j++)
        {
            int sum = 0;
            for (int l = -4; l <= 0; l++)
            {
                for (int k = -2; k <= 2; k++)
                {
                    index_cur = idx(j + k, i + l, W_image);
                    sum += DV[index_cur];
                }
            }
            index_cur = idx(j, i, W_image);
            Wn[index_cur] = ((sum) == 0 ? 1e20 : 1 / (1.0f * abs(sum)));
            //printf("wn=%f\n", Wn[index_cur]);
        }
    }

    for (int i = 0; i < H_image - 4; i++)
    {
        for (int j = 2; j < W_image - 2; j++)
        {
            int sum = 0;
            for (int l = 0; l <= 4; l++)
            {
                for (int k = -2; k <= 2; k++)
                {
                    index_cur = idx(j + k, i + l, W_image);
                    sum += DV[index_cur];
                }
            }
            index_cur = idx(j, i, W_image);
            Ws[index_cur] = ((sum) == 0 ? 1e20 : 1 / (1.0f * abs(sum)));
        }
    }

    for (int i = 2; i < H_image - 2; i++)
    {
        for (int j = 4; j < W_image; j++)
        {
            int sum = 0;
            for (int l = -2; l <= 2; l++)
            {
                for (int k = -4; k <= 0; k++)
                {
                    index_cur = idx(j + k, i + l, W_image);
                    sum += DH[index_cur];
                }
            }
            index_cur = idx(j, i, W_image);
            Ww[index_cur] = ((sum) == 0 ? 1e20 : 1 / (1.0f * abs(sum)));
        }
    }

    for (int i = 2; i < H_image - 2; i++)
    {
        for (int j = 0; j < W_image - 4; j++)
        {
            int sum = 0;
            for (int l = -2; l <= 2; l++)
            {
                for (int k = 0; k <= 4; k++)
                {
                    index_cur = idx(j + k, i + l, W_image);
                    sum += DH[index_cur];
                }
            }
            index_cur = idx(j, i, W_image);
            We[index_cur] = ((sum) == 0 ? 1e20 : 1 / (1.0f * abs(sum)));
        }
    }

    for (int i = 4; i < H_image - 4; i++)
    {
        for (int j = 4; j < W_image - 4; j++)
        {
            index_cur = idx(j, i, W_image);
            float v1 = Wn[index_cur] * 0.2 * (GRdeltaV[idx(j, i, W_image)] + GRdeltaV[idx(j, i - 1, W_image)] + GRdeltaV[idx(j, i - 2, W_image)] + GRdeltaV[idx(j, i - 3, W_image)] + GRdeltaV[idx(j, i - 4, W_image)]);
            float v2 = Ws[index_cur] * 0.2 * (GRdeltaV[idx(j, i, W_image)] + GRdeltaV[idx(j, i + 1, W_image)] + GRdeltaV[idx(j, i + 2, W_image)] + GRdeltaV[idx(j, i + 3, W_image)] + GRdeltaV[idx(j, i + 4, W_image)]);
            float v3 = We[index_cur] * 0.2 * (GRdeltaH[idx(j, i, W_image)] + GRdeltaH[idx(j - 1, i, W_image)] + GRdeltaH[idx(j - 2, i, W_image)] + GRdeltaH[idx(j - 3, i, W_image)] + GRdeltaH[idx(j - 4, i, W_image)]);
            float v4 = Ww[index_cur] * 0.2 * (GRdeltaH[idx(j, i, W_image)] + GRdeltaH[idx(j + 1, i, W_image)] + GRdeltaH[idx(j + 2, i, W_image)] + GRdeltaH[idx(j + 3, i, W_image)] + GRdeltaH[idx(j + 4, i, W_image)]);

            deltaGR[index_cur] = (int)((v1 + v2 + v3 + v4) / (Wn[index_cur] + Ws[index_cur] + We[index_cur] + Ww[index_cur]));

        }
    }

    for (int i = 4; i < H_image - 4; i++)
    {
        for (int j = 4; j < W_image - 4; j++)
        {
            index_cur = idx(j, i, W_image);
            float v1 = Wn[index_cur] * 0.2 * (GBdeltaV[idx(j, i, W_image)] + GBdeltaV[idx(j, i - 1, W_image)] + GBdeltaV[idx(j, i - 2, W_image)] + GBdeltaV[idx(j, i - 3, W_image)] + GBdeltaV[idx(j, i - 4, W_image)]);
            float v2 = Ws[index_cur] * 0.2 * (GBdeltaV[idx(j, i, W_image)] + GBdeltaV[idx(j, i + 1, W_image)] + GBdeltaV[idx(j, i + 2, W_image)] + GBdeltaV[idx(j, i + 3, W_image)] + GBdeltaV[idx(j, i + 4, W_image)]);
            float v3 = We[index_cur] * 0.2 * (GBdeltaH[idx(j, i, W_image)] + GBdeltaH[idx(j - 1, i, W_image)] + GBdeltaH[idx(j - 2, i, W_image)] + GBdeltaH[idx(j - 3, i, W_image)] + GBdeltaH[idx(j - 4, i, W_image)]);
            float v4 = Ww[index_cur] * 0.2 * (GBdeltaH[idx(j, i, W_image)] + GBdeltaH[idx(j + 1, i, W_image)] + GBdeltaH[idx(j + 2, i, W_image)] + GBdeltaH[idx(j + 3, i, W_image)] + GBdeltaH[idx(j + 4, i, W_image)]);

            deltaGB[index_cur] = (int)((v1 + v2 + v3 + v4) / (Wn[index_cur] + Ws[index_cur] + We[index_cur] + Ww[index_cur]));

        }
    }

    for (int i = extrasize; i < H_image - extrasize; i++)
    {
        for (int j = extrasize; j < W_image - extrasize; j++)
        {
            index_cur = idx(j, i, W_image);
            int ypos = i % 2;
            int xpos = j % 2;
            if ((ypos == 0) && (xpos == 0))//r
            {
                mosaicResult[index_cur + imagesize] = (u16)CLIP(raw_original[index_cur] + deltaGR[index_cur], 0, Bitmax);
            }
            else if ((ypos == 0) && (xpos == 1))//gr
            {
                mosaicResult[index_cur + imagesize] = (u16)raw_original[index_cur];
            }
            else if ((ypos == 1) && (xpos == 0))//gb
            {
                mosaicResult[index_cur + imagesize] = (u16)raw_original[index_cur];
            }
            else if ((ypos == 1) && (xpos == 1))//b
            {
                mosaicResult[index_cur + imagesize] = (u16)CLIP(raw_original[index_cur] + deltaGB[index_cur], 0, Bitmax);
            }
        }
    }
    // r b channel interpolate
    float prb[49] = { 0 };
    float sum_prb = 0;
    prb[2] = prb[4] = prb[14] = prb[2 * 7 + 6] = -1;
    prb[6 * 7 + 2] = prb[6 * 7 + 4] = prb[4 * 7] = prb[4 * 7 + 6] = -1;
    prb[2 * 7 + 2] = prb[2 * 7 + 4] = prb[4 * 7 + 2] = prb[4 * 7 + 4] = 10;
    for (int i = 0; i < 7 * 7; i++) sum_prb += prb[i];
    for (int i = 0; i < 7 * 7; i++) prb[i] = prb[i] / sum_prb;
    int index_f = 0;
    for (int i = 3; i < H_image - 3; i++)
    {
        for (int j = 3; j < W_image - 3; j++)
        {
            int ypos = i % 2;
            int xpos = j % 2;
            int tempdetagR = 0;
            int tempdetagB = 0;
            index_cur = idx(j, i, W_image);
            if (((0 == ypos) && (0 == xpos)) || ((1 == ypos) && (1 == xpos)))
            {
                for (int m = -3; m <= 3; m++)
                {
                    for (int n = -3; n <= 3; n++)
                    {
                        int index_cur_cov = idx(j + n, i + m, W_image);
                        index_f = (m + 3) * 7 + n + 3;
                        tempdetagR += prb[index_f] * deltaGR[index_cur_cov];
                        tempdetagB += prb[index_f] * deltaGB[index_cur_cov];
                    }
                }
            }
            if ((0 == ypos) && (0 == xpos))
            {
                mosaicResult[index_cur + 2 * imagesize] = (u16)CLIP((int)mosaicResult[index_cur + imagesize] - tempdetagB, 0, Bitmax);


            }
            if ((1 == ypos) && (1 == xpos))
            {
                mosaicResult[index_cur] = (u16)CLIP((int)mosaicResult[index_cur + imagesize] - tempdetagR, 0, Bitmax);
                //	printf("mosaicResult[index_cur]=%d\n", mosaicResult[index_cur]);
            }


        }
    }
    float now_kernel[3][3] = { {0,1,0},{1,0,1},{0,1,0} };
    int* now_deltaGR = (int*)calloc(imagesize, sizeof(int));
    int* now_deltaGB = (int*)calloc(imagesize, sizeof(int));
    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            int ypos = i % 2;
            int xpos = j % 2;
            index_cur = idx(j, i, W_image);
            now_deltaGR[index_cur] = ((int)mosaicResult[index_cur + imagesize] - (int)mosaicResult[index_cur]);
            now_deltaGB[index_cur] = ((int)mosaicResult[index_cur + imagesize] - (int)mosaicResult[index_cur + 2 * imagesize]);

        }
    }
    for (int i = 1; i < H_image - 1; i++)
    {
        for (int j = 1; j < W_image - 1; j++)
        {
            int ypos = i % 2;
            int xpos = j % 2;
            index_cur = idx(j, i, W_image);
            int index_f = 0;
            int temp = 0;
            int temp2 = 0;
            for (int m = -1; m <= 1; m++)
            {
                for (int n = -1; n <= 1; n++)
                {
                    temp += now_deltaGR[idx(j + n, i + m, W_image)] * now_kernel[1 + m][1 + n];
                    temp2 += now_deltaGB[idx(j + n, i + m, W_image)] * now_kernel[1 + m][1 + n];
                }
            }
            temp = temp / 4;
            temp2 = temp2 / 4;
            if ((0 == ypos) && (1 == xpos))
            {
                mosaicResult[index_cur] = (u16)CLIP((int)mosaicResult[index_cur + imagesize] - temp, 0, Bitmax);
                //	printf("mosaicResult[index_cur]=%d\n", mosaicResult[index_cur]);
                mosaicResult[index_cur + 2 * imagesize] = (u16)CLIP((int)mosaicResult[index_cur + imagesize] - temp2, 0, Bitmax);
            }
            if ((1 == ypos) && (0 == xpos))
            {
                mosaicResult[index_cur] = (u16)CLIP((int)mosaicResult[index_cur + imagesize] - temp, 0, Bitmax);
                //	printf("mosaicResult[index_cur]=%d\n", mosaicResult[index_cur]);
                mosaicResult[index_cur + 2 * imagesize] = (u16)CLIP((int)mosaicResult[index_cur + imagesize] - temp2, 0, Bitmax);
            }

        }
    }



    return 0;
}
#endif
#include <algorithm> // 用于 std::sort

// 辅助函数：简单的9个元素排序获取中值
#include <vector>
#include <algorithm>

// 辅助函数：3x3 中值滤波（用于消除彩虹纹）
#define s2(a, b) { int t = (a); if ((b) < t) { (a) = (b); (b) = t; } }
inline int fast_median9(int* n) {
    s2(n[1], n[2]); s2(n[4], n[5]); s2(n[7], n[8]);
    s2(n[0], n[1]); s2(n[3], n[4]); s2(n[6], n[7]);
    s2(n[1], n[2]); s2(n[4], n[5]); s2(n[7], n[8]);
    s2(n[0], n[3]); s2(n[5], n[8]); s2(n[4], n[7]);
    s2(n[3], n[6]); s2(n[1], n[4]); s2(n[2], n[5]);
    s2(n[4], n[7]); s2(n[4], n[2]); s2(n[6], n[4]);
    s2(n[4], n[2]); return n[4];
}

int Interpolation(int H_image, int W_image, u16* u16raw_original, u16* mosaicResult, stISPParams* gISPparam)
{
    int imagesize = H_image * W_image;
    int Bitmax = (1 << gISPparam->demosaic_Param.Demosaicbit) - 1;
    float eps = 1.0f;
    // 1. 内存优化：一次性申请连续内存，减少碎片和分配耗时
    // 使用 malloc 提高速度，配合 memset 清空关键区域（防止边缘脏数据）
    int* buffer = (int*)malloc(imagesize * 5 * sizeof(int));
    if (!buffer) return -1;

    int* G = buffer;
    int* R = buffer + imagesize;
    int* B = buffer + imagesize * 2;
    int* dGR = buffer + imagesize * 3;
    int* dGB = buffer + imagesize * 4;

    // 2. Green 插值优化：循环展开与索引预计算
    for (int i = 2; i < H_image - 2; i++) {
        int row_idx = i * W_image;
        int ypos = i % 2;
        for (int j = 2; j < W_image - 2; j++) {
            int cur = row_idx + j;
            if (ypos != (j % 2)) {
                G[cur] = (int)u16raw_original[cur];
            }
            else {
                // 缓存邻域访问
                int raw_c = u16raw_original[cur];
                int raw_l = u16raw_original[cur - 1];
                int raw_r = u16raw_original[cur + 1];
                int raw_u = u16raw_original[cur - W_image];
                int raw_d = u16raw_original[cur + W_image];

                float hGrad = abs(raw_l - raw_r) + abs(2 * raw_c - u16raw_original[cur - 2] - u16raw_original[cur + 2]);
                float vGrad = abs(raw_u - raw_d) + abs(2 * raw_c - u16raw_original[cur - 2 * W_image] - u16raw_original[cur + 2 * W_image]);

                float Wh = 1.0f / (hGrad * hGrad + eps);
                float Wv = 1.0f / (vGrad * vGrad + eps);

                float Gh = (raw_l + raw_r) * 0.5f + (2 * raw_c - u16raw_original[cur - 2] - u16raw_original[cur + 2]) * 0.25f;
                float Gv = (raw_u + raw_d) * 0.5f + (2 * raw_c - u16raw_original[cur - 2 * W_image] - u16raw_original[cur + 2 * W_image]) * 0.25f;

                G[cur] = (int)CLIP((Gh * Wh + Gv * Wv) / (Wh + Wv), 0, Bitmax);
            }
        }
    }

    // 3. 固定相位插值优化：减少分支判断次数
    for (int i = 2; i < H_image - 2; i++) {
        int row_idx = i * W_image;
        int ypos = i % 2;
        for (int j = 2; j < W_image - 2; j++) {
            int cur = row_idx + j;
            int xpos = j % 2;
            int g_val = G[cur];

            if (ypos == 0 && xpos == 0) { // R位置
                R[cur] = (int)u16raw_original[cur];
                int diffB = ((G[cur - W_image - 1] - u16raw_original[cur - W_image - 1]) + (G[cur - W_image + 1] - u16raw_original[cur - W_image + 1]) +
                    (G[cur + W_image - 1] - u16raw_original[cur + W_image - 1]) + (G[cur + W_image + 1] - u16raw_original[cur + W_image + 1])) >> 2;
                dGR[cur] = g_val - R[cur];
                dGB[cur] = diffB;
            }
            else if (ypos == 1 && xpos == 1) { // B位置
                B[cur] = (int)u16raw_original[cur];
                int diffR = ((G[cur - W_image - 1] - u16raw_original[cur - W_image - 1]) + (G[cur - W_image + 1] - u16raw_original[cur - W_image + 1]) +
                    (G[cur + W_image - 1] - u16raw_original[cur + W_image - 1]) + (G[cur + W_image + 1] - u16raw_original[cur + W_image + 1])) >> 2;
                dGB[cur] = g_val - B[cur];
                dGR[cur] = diffR;
            }
            else if (ypos == 0 && xpos == 1) { // Gr位置
                int diffR = ((G[cur - 1] - u16raw_original[cur - 1]) + (G[cur + 1] - u16raw_original[cur + 1])) >> 1;
                int diffB = ((G[cur - W_image] - u16raw_original[cur - W_image]) + (G[cur + W_image] - u16raw_original[cur + W_image])) >> 1;
                dGR[cur] = diffR;
                dGB[cur] = diffB;
            }
            else { // Gb位置
                int diffB = ((G[cur - 1] - u16raw_original[cur - 1]) + (G[cur + 1] - u16raw_original[cur + 1])) >> 1;
                int diffR = ((G[cur - W_image] - u16raw_original[cur - W_image]) + (G[cur + W_image] - u16raw_original[cur + W_image])) >> 1;
                dGB[cur] = diffB;
                dGR[cur] = diffR;
            }
        }
    }

    // 4. 中值滤波优化：展开邻域读取，直接使用快排网络
    u16* outR = mosaicResult;
    u16* outG = mosaicResult + imagesize;
    u16* outB = mosaicResult + imagesize * 2;

    for (int i = 2; i < H_image - 2; i++) {
        int row_idx = i * W_image;
        for (int j = 2; j < W_image - 2; j++) {
            int cur = row_idx + j;
            int winR[9], winB[9];

            // 邻域预计算
            int r_m = row_idx - W_image, r_p = row_idx + W_image;
            int j_m = j - 1, j_p = j + 1;

            winR[0] = dGR[r_m + j_m]; winR[1] = dGR[r_m + j]; winR[2] = dGR[r_m + j_p];
            winR[3] = dGR[row_idx + j_m]; winR[4] = dGR[cur]; winR[5] = dGR[row_idx + j_p];
            winR[6] = dGR[r_p + j_m]; winR[7] = dGR[r_p + j]; winR[8] = dGR[r_p + j_p];

            winB[0] = dGB[r_m + j_m]; winB[1] = dGB[r_m + j]; winB[2] = dGB[r_m + j_p];
            winB[3] = dGB[row_idx + j_m]; winB[4] = dGB[cur]; winB[5] = dGB[row_idx + j_p];
            winB[6] = dGB[r_p + j_m]; winB[7] = dGB[r_p + j]; winB[8] = dGB[r_p + j_p];

            int medR = fast_median9(winR);
            int medB = fast_median9(winB);

            int g_val = G[cur];
            outR[cur] = (u16)CLIP(g_val - medR, 0, Bitmax);
            outG[cur] = (u16)g_val;
            outB[cur] = (u16)CLIP(g_val - medB, 0, Bitmax);
        }
    }

    free(buffer);
    return 0;
}
void antifalsecolor(unsigned short* interpolated, stISPParams* gISPparam)
{
    int H_image = gISPparam->rawinfo.H;
    int W_image = gISPparam->rawinfo.W;
    int thr_y = gISPparam->demosaic_Param.thr_y;
    int thr_c = gISPparam->demosaic_Param.thr_c;
    int false_k = gISPparam->demosaic_Param.false_k;
    int MaxValue = (1 << gISPparam->demosaic_Param.Demosaicbit) - 1;
    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            interpolated[i * W_image + j + H_image * W_image * 2] = CLIP(interpolated[i * W_image + j + H_image * W_image * 2] , 0, MaxValue);
            interpolated[i * W_image + j] = CLIP(interpolated[i * W_image + j], 0, MaxValue);
            interpolated[i * W_image + j + H_image * W_image] = CLIP(interpolated[i * W_image + j + H_image * W_image], 0, MaxValue);

        }
    }

    int kersize = 3;

    double* textureMap = (double*)malloc(H_image * W_image * (sizeof(double)));
    memset(textureMap, 0, H_image * W_image * (sizeof(unsigned short)));

    unsigned short* result = (unsigned short*)malloc(3 * H_image * W_image * (sizeof(unsigned short)));
    memset(result, 0, 3 * H_image * W_image * (sizeof(unsigned short)));

    int* Ycbcr = (int*)malloc(3 * H_image * W_image * (sizeof(int)));
    memset(Ycbcr, 0, 3 * H_image * W_image * (sizeof(int)));



    unsigned short* caMap = (unsigned short*)malloc(H_image * W_image * (sizeof(unsigned short)));
    memset(caMap, 0, H_image * W_image * (sizeof(unsigned short)));

    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            unsigned short R = interpolated[i * W_image + j + H_image * W_image * 2];
            unsigned short G = interpolated[i * W_image + j];
            unsigned short B = interpolated[i * W_image + j + H_image * W_image];
            Ycbcr[i * W_image + j] = (1224 * R + 2404 * G + 468 * B) / 4096;
            Ycbcr[i * W_image + j + H_image * W_image] = 2048 + (-690 * R - 1354 * G + 2048 * B) / 4096;
            Ycbcr[i * W_image + j + 2 * H_image * W_image] = 2048 + (2048 * R - 1603 * G - 311 * B) / 4096;
        }
    }

    for (int i = kersize / 2; i < H_image - kersize / 2; i++)
    {
        for (int j = kersize / 2; j < W_image - kersize / 2; j++)
        {
            double isum = 0;
            double idelta = 0;
            double isum_sq = 0;
            for (int k = -kersize / 2; k <= kersize / 2; k++)
            {
                for (int l = -kersize / 2; l <= kersize / 2; l++)
                {
                    isum += Ycbcr[(i + k) * W_image + j + l];
                    isum_sq += Ycbcr[(i + k) * W_image + j + l] * Ycbcr[(i + k) * W_image + j + l];
                }
            }
            isum = isum / (kersize * kersize);
            idelta = (isum_sq / (kersize * kersize)) - isum * isum;

            textureMap[i * W_image + j] = sqrt(idelta);
        }
    }

    for (int i = kersize / 2; i < H_image - kersize / 2; i++)
    {
        for (int j = kersize / 2; j < W_image - kersize / 2; j++)
        {
            double isum = 0;
            for (int k = -kersize / 2; k <= kersize / 2; k++)
            {
                for (int l = -kersize / 2; l <= kersize / 2; l++)
                {
                    int cr = Ycbcr[(i + k) * W_image + j + l + 2 * H_image * W_image];
                    int cb = Ycbcr[(i + k) * W_image + j + l + H_image * W_image];
                    int cr_center = Ycbcr[i * W_image + j + 2 * H_image * W_image];
                    int cb_center = Ycbcr[i * W_image + j + H_image * W_image];

                    isum += sqrt((cr - cr_center) * (cr - cr_center) + (cb - cb_center) * (cb - cb_center));

                }
            }
            caMap[i * W_image + j] = isum;
        }
    }
    unsigned short* mask = (unsigned short*)malloc(H_image * W_image * (sizeof(unsigned short)));
    memset(mask, 0, H_image * W_image * (sizeof(unsigned short)));
    for (int i = 0; i < H_image; i++)
    {
        for (int j = 0; j < W_image; j++)
        {
            int ca = caMap[i * W_image + j];
            int text = textureMap[i * W_image + j];
            double texture = textureMap[i * W_image + j];
            if ((text > thr_y) && (ca > thr_c))
            {
                mask[i * W_image + j] = 255;
                int r = interpolated[i * W_image + j + H_image * W_image * 2];
                int g = interpolated[i * W_image + j];
                int b = interpolated[i * W_image + j + H_image * W_image];

                int res = (r + g + b) / 3;
                interpolated[i * W_image + j + H_image * W_image * 2] = res * false_k + (1 - false_k) * r;
                interpolated[i * W_image + j] = res * false_k + (1 - false_k) * g;
                interpolated[i * W_image + j + H_image * W_image] = res * false_k + (1 - false_k) * b;

            }

        }
    }

}
void Reorderbayer(stISPParams* gISPparam, u16 *indata_extern,u16* indata)
{

    u8 x_offset = 0;
    u8 y_offset = 0;
    eBayerPattern bayerPattern = gISPparam->rawinfo.BayerType;

    switch (bayerPattern)
    {
    case RGGB:

        break;

    case GRBG:
        x_offset = 1;
        break;
    case BGGR:
        x_offset = 1;
        y_offset = 1;
        break;

    case GBRG:
        y_offset = 1;
        break;
    case CRGC:
        x_offset = 1;
        break;
    }
    u16 H = gISPparam->rawinfo.H;
    u16 W = gISPparam->rawinfo.W;
    u16 data_externnum=gISPparam->raw_extern.entern_num;
    for (int i=0;i<H;i++)
    {
        for (int j=0;j<W;j++)
        {
            indata[i*W+j] = indata_extern[(i + y_offset)*(2* data_externnum+W)+j+x_offset];
        }
    }

}
void switchChannel(u16* inrgbdata, u16* outrgbdata,int imagesize)
{
    //rgb->gbr
    for (int i=0;i<imagesize;i++)
    {
        u16 r = inrgbdata[i];
        u16 g = inrgbdata[i+imagesize];
        u16 b = inrgbdata[i + 2*imagesize];

        inrgbdata[i] = g;
        inrgbdata[i+imagesize] = b;
        inrgbdata[i+2*imagesize] = r;
    }

}
int Run_demosaic(stISPParams* gISPparam, u16* indata_extern, u16* rgbdata)
{
    int H_image = gISPparam->rawinfo.H;
    int W_image = gISPparam->rawinfo.W;
    gW_image = W_image;
    int imgSize = H_image * W_image;

    u16* Dir_map = (u16*)malloc((H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(u16)));

    u16* interpolated = (u16*)malloc(3 * imgSize * (sizeof(u16)));

    memset(Dir_map, 0, (H_image + extern_num * 2) * (W_image + extern_num * 2));
    u16* raw_externed = (u16*)malloc((H_image + extern_num * 2) * (W_image + extern_num * 2) * (sizeof(u16)));

    u16* indata = (u16*)malloc(imgSize * (sizeof(u16)));
    Reorderbayer(gISPparam, indata_extern, indata);


	//fRaw_extern(indata, raw_externed, H_image, W_image);

	//fOri_Dir(raw_externed, Dir_map, gISPparam);
    //interpolate(raw_externed, Dir_map, rgbdata, gISPparam);
    Interpolation(H_image, W_image, indata, rgbdata, gISPparam);//1-r,2-g,3-b
    switchChannel(rgbdata, rgbdata, H_image* W_image);
    antifalsecolor(rgbdata, gISPparam);
    /*rccg test*/
#if RCCG
    for (int i = 0; i< imgSize; i++)
    {
        int Cpix = rgbdata[i];
        
        int Gpix= rgbdata[i+ imgSize];
        int Rpix = rgbdata[i + 2*imgSize];
       // int B = Cpix * 1.054 - 1*Rpix +0.44 * Gpix;
        int B = Cpix * 2.38035 - 1.03166 *Rpix - 1.024703 * Gpix;
      //  printf("B=%d\n", B);
        B = CLIP(B, 0, 65535);
        rgbdata[i] = Gpix;
        rgbdata[i + imgSize] = B;
        rgbdata[i + 2 * imgSize] = Rpix;

    }
#endif
    /***********/
    if (gISPparam->rawinfo.ispmode == ISPCombined)
    {
        debug_write16bit_image(H_image, W_image, rgbdata, "1-demosaic.jpg");
    }
    else
    {
        debug_write12bit_image(H_image, W_image, rgbdata, "1-demosaic.jpg");
    }
    
 
    return 0;
}