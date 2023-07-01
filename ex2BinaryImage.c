#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//色空間変換のマクロ定義
#define CH 3
#define Ych 0

#define ROW 3
#define COL 3

//二値画像生成のブロックサイズ定義
#define FSIZE 7   //フィルタサイズ
#define BSIZE 35   //ブロックサイズ


/*=====================*
 *プロトタイプ宣言      *
 *=====================**/

void get_data(int argc, char *argv[]);

int show_data(int start, int end, int status);

int calc_data(int start, int end);

void processing(void);

void put_data(int argc, char *argv[]);

void output_image(void);

void rgb_to_ybr(void);

void ybr_to_rgb(void);

void output_excelfile(int row, int col);

void conversion_ybr(void);

/*=====================*
 *グローバル変数        *
 *=====================**/
//ヘッダー内容
unsigned char header[54];

//画像データ格納配列
unsigned char imgin[3][512][512];
//画像データ格納配列2
unsigned char imgprc[3][512][512];
//画像データコピー格納配列
unsigned char imgout[3][512][512];

//ヘッダー各種情報の格納
int filesize;
int offset;
int width;
int height;
int bits;
int inserted_bits;


//色空間変換の情報格納

//RGBからYCbCrへの変換行列
double matrix_rgb_to_ybr[ROW][COL] =
{
    { 0.2990, 0.5870, 0.1140},
    {-0.1687,-0.3313, 0.5000},
    { 0.5000,-0.4187,-0.0813}
};

//YCbCrからRGBへの変換行列
double matrix_ybr_to_rgb[ROW][COL] =
{
    { 1.0000, 0.0000, 1.4020},
    { 1.0000,-0.3441,-0.7141},
    { 1.0000, 1.7720, 0.0000}
};

int rgb_in[CH]; // 入力RGB信号(整数値)
int ybr[CH]; // YCbCr信号(整数値)
int rgb_out[CH]; // 出力RGB信号(整数値)
double dtemp[CH];   //YCbCr信号への変更情報
int ch;
int itemp;

/**
 * main関数: 各関数の呼び出し
*/
int main(int argc, char *argv[]){
  
    get_data(argc, argv);

    conversion_ybr();

    processing();   //主処理
    
    ybr_to_rgb();

    put_data(argc, argv);

    return 0;
}

/**
 * 画像データ取得
*/
void get_data(int argc, char *argv[]){

    FILE *fp;

    char filename[256];

    //コマンドライン引数から入力ファイル名を取得
    strcpy(filename, argv[1]);
    printf("入力ファイル名: %s\n", filename);

    fp = fopen(filename, "rb");
    if(fp == NULL){
        printf("%sをオープンできません. \n", filename);
        exit(1);
    }

    printf("%sをオープンしました. \n", filename);

    //配列にヘッダ内容を全て格納
    for(int i = 0; i < 54; i++){
        header[i] = getc(fp);
    }
        
    //---ファイルサイズ---
    filesize = show_data(2, 5, 1);


    //---オフセット---
    offset = show_data(10, 13, 1);

        
    //---画像の幅---
    width = show_data(18, 21, 1);
    printf("width --- %d ---\n", width);

    //---画像の高さ---
    height = show_data(22, 25, 1);
    printf("height --- %d ---\n", height);

    //---１画素あたりのビット数---
    bits = show_data(28, 29, 1);

    //---挿入ビット数---
    inserted_bits = filesize - offset - width * height * (bits/8);
        
    /**
     * 画像データ部の読み込み
     */
    for(int i = height - 1; i >= 0; i--){   //縦ループ, 上から下
        for(int j = 0; j < width; j++){ //横ループ, 左から右(+)
            for(int k = 0; k < 3; k++){ //3色分, B G R(+)
                imgin[k][j][i] = getc(fp);
            }
        }
    }

    fclose(fp);
    printf("%sをクローズしました.\n\n", filename);
}


/**
 * get_data内での指定があった場合のデータ表示
*/
int show_data(int start, int end, int status){
    int result = 0;
    if(status == 1){ //その後の計算が必要な場合
        result = calc_data(start, end);
    }
    return result;
}


/**
 * 挿入ビット数の計算
 */
int calc_data(int start, int end){
    int bytes = end - start + 1;
    int i;
    int result = header[end];
    
    for(i = bytes - 2; i >= 0; i--){
        result <<= 8;
        result += header[start + i];
    }
    
    return result;
}

/**
 * 二値画像生成処理
*/
void processing(void){

    double tmp;    //臨時格納用

    double filter_gaussian[FSIZE][FSIZE];

    double pi = 4.0 * atan(1.0);    //π=の3.14...の近似
    double sigma = 1.7; //σの定義

    double calc_tmp = (1 / (2 * pi * pow(sigma, 2)));

    printf("calc_tmp = %f\n", calc_tmp);

    /*ガウシアンフィルタ生成(BSIZE*BSIZE)*/
    for(int i = 0; i < FSIZE; i++) {
        printf("a\n");
        
        for(int j = 0; j < FSIZE; j++) {
            filter_gaussian[i][j] = calc_tmp * exp(-1 * (pow(i - (int)(FSIZE / 2), 2) + pow(j - (int)(FSIZE / 2), 2)) / (2 * pow(sigma, 2)));
        }
    }

    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){

            tmp = 0;

            for(int s = 0; s < FSIZE; s++){
                for(int t = 0; t < FSIZE; t++){
                    //領域外(プラス画像にオーバー)であるとき[鏡像変換]
                    if((j - (int)(FSIZE / 2) + t) >= width || (i - (int)(FSIZE / 2) + t) >= height){   //領域外(プラス方向にオーバー)であるとき
                        tmp += (double)imgin[2][abs(j - (int)(FSIZE / 2) + t) - (abs(j - (int)(FSIZE / 2) + t) - width)][abs(i - (int)(FSIZE / 2) + s) - (abs(i - (int)(FSIZE / 2) + s) - height)] * filter_gaussian[t][s];
                    } else {        //領域内　もしくは　領域外(マイナス方向にオーバー)であるとき
                        tmp += (double)imgin[2][abs(j - (int)(FSIZE / 2) + t)][abs(i - (int)(FSIZE / 2) + s)] * filter_gaussian[t][s];
                    }
                }
            } 

            imgprc[2][j][i] = (int)round(tmp);
            imgprc[1][j][i] = 128;
            imgprc[0][j][i] = 128;
            
        }
    }

    //差分をとる
    /*
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            imgout[2][j][i] = imgprc[2][j][i] - imgin[2][j][i];
            imgout[1][j][i] = imgprc[1][j][i];
            imgout[0][j][i] = imgprc[0][j][i];
        }
    }
    */

    //二値
    double threshold;
    
    //全画素分回す
    for(int i = 0; i < height; i++){    //縦ループ
        for(int j = 0; j < width; j++){ //横ループ
            //閾値を初期化
            threshold = 0;
            
            //ブロック参照
            //順番に代入... 対象画素を中心に周囲BSIZE分を格納
            for(int k = 0; k < BSIZE; k++) {     //縦ループ
                for(int l = 0; l < BSIZE; l++) { //横ループ
                    //領域外(プラス画像にオーバー)であるとき[鏡像変換]
                    if(j - (int)(BSIZE / 2) + l >= width || i - (int)(BSIZE / 2) + k >= height) {
                        threshold += imgprc[2][abs(j - (int)(BSIZE / 2) + l) - (abs(j - (int)(BSIZE / 2) + l) - width)][abs(i - (int)(BSIZE / 2) + k) - (abs(i - (int)(BSIZE / 2) + k) - height)];
                    } else {    //領域内 もしくは 領域外(マイナス方向にオーバー)であるとき[鏡像変換]
                        threshold += imgprc[2][j - (int)(BSIZE / 2) + l][i - (int)(BSIZE / 2) + k];
                    }
                }
            }
            //平均をとる
            threshold /= pow(BSIZE, 2);

            if(j == 5) printf("th: %f, img: %d\n", threshold, imgprc[2][j][i]);

            if(threshold < imgin[2][j][i]) {   //平均が元より小さいとき(<): 白
                imgout[2][j][i] = 235;
                imgout[1][j][i] = 128;
                imgout[0][j][i] = 128;
            } else {                           //平均がもとより大きいとき(<): 黒
                imgout[2][j][i] = 16;
                imgout[1][j][i] = 128;
                imgout[0][j][i] = 128;
            }
        }
    }


}


/**
 * 画像の出力
*/
void put_data(int argc, char *argv[]){
    FILE *fp;
    char filename[256];
    strcpy(filename, argv[2]);

    printf("出力ファイル名: %s\n", filename);
        
    fp = fopen(filename, "wb");
    printf("%sをオープンしました.\n", filename);

    //ヘッダー部分格納
    for(int i = 0; i < 54; i++){
        fputc(header[i], fp);
    }

    //画像部分格納
    for(int i = height - 1; i >= 0; i--){
        for(int j = 0; j < width; j++){
            for(int k = 0; k < 3; k++){
                fputc(imgout[k][j][i], fp);
            }
        }
    }

    //4バイトアラインメントに必要なビットを挿入
    for(int i = 0; i < inserted_bits; i++){
        fputc('\0', fp);
    }

    fclose(fp);
    printf("%sをクローズしました.\n", filename);
}

/**
 * [色空間変換][固定値への置換]
 * YCbCr信号の変換値を入力
*/
void conversion_ybr(void){

    int mode_y = 1;     //yの変換ステータス
    int mode_Cb = 0;    //Cbの変換ステータス
    int mode_Cr = 0;    //Crの変換ステータス


    /*int rgb_in[3];への読み込み
     *  rgb_in[0]: R
     *  rgb_in[1]: G
     *  rgb_in[2]: B
     */

    for(int i = 0; i < height; i++){    //縦ループ, 下から上(+)
        for(int j = 0; j < width; j++){ //横ループ, 左から右(+)
            for(int k = 2; k >= 0; k--){//色ループ, R G B(-)
                rgb_in[2 - k] = imgin[k][j][i]; //[格納]rgb_inは0~2でRGB imginは0~2でCrCbY
            }

                

            //--- RGB信号(整数値)からYCbCr信号(実数値)への変換 ---
            for(ch = 0; ch < CH; ch++){
                dtemp[ch] = 0.0;
                for (int i = 0; i < COL; i++){
                    dtemp[ch] += matrix_rgb_to_ybr[ch][i] * (double)rgb_in[i];
                }
            }
                
                
            //--- YCbCr信号(実数値)からYCbCr信号(整数値)への変換 ---
            for (ch = 0; ch < CH; ch++){
            // 四捨五入
                if (dtemp[ch] > 0.0){
                    itemp = (int)(dtemp[ch] + 0.5);
                } else {
                    itemp = (int)(dtemp[ch] - 0.5);
                }

                // Cb,Cr信号にオフセット値128を加える
                if (ch != Ych){
                    itemp += 128;
                }

                // 信号値を0~255の範囲内に制限する
                if (itemp > 255){
                    itemp = 255;
                } else if (itemp < 0){
                    itemp = 0;
                }
                /* YCbCr信号値(整数値)を格納
                * 各パターンにおいて動作を変更:
                * ch == 0(Y) かつ mode_y == 0の場合(固定値) 128に固定
                * ...以下同様
                */
                if((mode_y == 0 && ch == 0) || (mode_Cb == 0 && ch == 1) || (mode_Cr == 0 && ch == 2)){
                    ybr[ch] = 128;
                } else {
                    ybr[ch] = itemp;
                }
            }
                

            //配列int imgin[3][512][512]への格納
            for(int k = 2; k >= 0; k--){
                imgin[k][j][i] = ybr[2 - k];
            }

        }
    }

}

/**
 * [色空間変換]
 * YCbCrからRGBへの変換
 * 各種変数はカウンター変数以外グローバル変数として宣言
*/
void ybr_to_rgb(void){
    

    /*int rgb_out[3];への読み込み
        rgb_in[0]: R
        rgb_in[1]: G
        rgb_in[2]: B
    */


    for(int i = 0; i < height; i++){    //縦ループ, 下から上(+)
        for(int j = 0; j < width; j++){ //縦ループ, 左から右(+)
            for(int k = 2; k >= 0; k--){//色ループ, BGR(-)
                ybr[2 - k] = imgout[k][j][i]; //[格納]ybrは0~2でRGB imginは0~2でBGR
            }            
            //--- YCbCr信号(整数値)からRGB信号(実数値)への変換 ---
            for (ch = 0; ch < CH; ch++){
                dtemp[ch] = 0.0;
                for (int l = 0; l < COL; l++){
                    if (l == Ych){
                        dtemp[ch] += matrix_ybr_to_rgb[ch][l] * (double)ybr[l];
                    }else {// Cb,Cr信号は,オフセット値128を減ずる
                        dtemp[ch] += matrix_ybr_to_rgb[ch][l] * (double)(ybr[l] - 128);
                    }
                }
            }

            //--- RGB信号(実数値)からRGB信号(整数値)への変換 ---
            for (ch = 0; ch < CH; ch++){
                // 四捨五入
                if (dtemp[ch] > 0.0){
                    itemp = (int)(dtemp[ch] + 0.5);
                } else {
                    itemp = (int)(dtemp[ch] - 0.5);
                }
                // 信号値を0~255の範囲内に制限する
                if (itemp > 255){
                    itemp = 255;
                } else if (itemp < 0){
                    itemp = 0;
                }
                // RGB信号値(整数値)を格納
                rgb_out[ch] = itemp;
            }

            //配列int imgout[3][512][512]への格納
            for(int k = 2; k >= 0; k--){
                imgout[k][j][i] = rgb_out[2 - k];
            }
        }
    }
}