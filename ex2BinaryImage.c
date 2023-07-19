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
#define FSIZE 15  //フィルタサイズ
#define BSIZE 15   //ブロックサイズ


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
//画像データ格納配列3
unsigned char imgprc2[20][3][512][512];
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
    double sigma = 3.0; //σの定義

    double calc_tmp = (1 / (2 * pi * pow(sigma, 2)));

    /*ガウシアンフィルタ生成(FSIZE*FSIZE)*/
    for(int i = 0; i < FSIZE; i++) {
        
        for(int j = 0; j < FSIZE; j++) {
            filter_gaussian[i][j] = calc_tmp * exp(-1 * (pow(i - (int)(FSIZE / 2), 2) + pow(j - (int)(FSIZE / 2), 2)) / (2 * pow(sigma, 2)));
        }
    }

    /*
    //ガウシアンフィルタ適用作業中
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
    */
    

    //差分をとる
    /*
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            imgprc[2][j][i] = imgprc[2][j][i] - imgin[2][j][i];
            imgprc[1][j][i] = imgin[1][j][i];
            imgprc[0][j][i] = imgin[0][j][i];
        }
    }
    */
    

    //imgprcに代入(なにもしないとき)
    
    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            for(int k = 0; k < 3; k++) {
                imgprc[k][j][i] = imgin[k][j][i];
            }
        }
    }

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
                        threshold += imgprc[2][abs(j - (int)(BSIZE / 2) + l) - (abs(j - (int)(BSIZE / 2) + l) - width)][abs(i - (int)(BSIZE / 2) + k) - (abs(i - (int)(BSIZE / 2) + k) - height)]* filter_gaussian[l][k];
                    } else {    //領域内 もしくは 領域外(マイナス方向にオーバー)であるとき[鏡像変換]
                        threshold += imgprc[2][j - (int)(BSIZE / 2) + l][i - (int)(BSIZE / 2) + k]* filter_gaussian[l][k];
                    }
                }
            }
            
            //平均をとる
            //正規分布の重み付きである場合はそもそも割る数が含まれているためこの作業はなし
            //threshold /= pow(BSIZE, 2);

            //二値化
            if(threshold < imgprc[2][j][i]) {   //平均が元より小さいとき(<): 白
                imgprc2[0][2][j][i] = 235;
                imgprc2[0][1][j][i] = 128;
                imgprc2[0][0][j][i] = 128;
            } else {                           //平均がもとより大きいとき(<): 黒
                imgprc2[0][2][j][i] = 16;
                imgprc2[0][1][j][i] = 128;
                imgprc2[0][0][j][i] = 128;
            }
        }
    }

    int times;  //オープニングの回数指定
    int mode;   //近傍モード指定
    int termination_status; //2重以上のループ脱出status

    printf("オープニングの回数指定: ");
    scanf("%d", &times);
    printf("近傍指定(1->4, 2->8): ");
    scanf("%d", &mode);


    //オープニング処理
    //--収縮--
    /* 例: 元の二値画像はimgprc2[0]に入っている.
     * 格納はimtprc2[1]から...
     * 例を挙げると: times = 10の時(10回ずつの時)
     * 収縮結果の最終格納先はimgprc2[10]である.
     */
    for(int num = 1; num <= times; num++) {  //回数ループ
    //最も端の画素は今回は考えない.
        for(int i = 1; i < height - 1; i++) {   //たて
            for(int j = 1; j < width - 1; j++) {    //よこ
                if(mode == 1) { //4近傍
                    //1つ前の結果を参照して処理:4近傍
                    //黒画素の4近傍を探索して1つでも白だったら白
                    if(imgprc2[num - 1][2][j][i] <= 16) {
                        if(imgprc2[num - 1][2][j+1][i] >= 235 || imgprc2[num - 1][2][j-1][i] >= 235 || imgprc2[num - 1][2][j][i+1] >= 235 || imgprc2[num - 1][2][j][i-1] >= 235) {
                            imgprc2[num][2][j][i] = 235;
                            imgprc2[num][1][j][i] = 128;
                            imgprc2[num][0][j][i] = 128;
                            //printf("いいね！");
                        } else {
                        imgprc2[num][2][j][i] = imgprc2[num - 1][2][j][i];
                        imgprc2[num][1][j][i] = imgprc2[num - 1][1][j][i];
                        imgprc2[num][0][j][i] = imgprc2[num - 1][0][j][i];
                        }
                    } else {
                        imgprc2[num][2][j][i] = imgprc2[num - 1][2][j][i];
                        imgprc2[num][1][j][i] = imgprc2[num - 1][1][j][i];
                        imgprc2[num][0][j][i] = imgprc2[num - 1][0][j][i];
                    }
                } else if(mode == 2) {  //8近傍
                    termination_status = 0;
                    for(int k = -1; k <= 1; k++){   //縦
                        for(int l = -1; l <= 1; l++) {  //横
                            if(l == 0 && k == 0) {
                                //なにもしない
                            } else if(imgprc2[num - 1][2][j + l][i + k] >= 235 && imgprc2[num - 1][2][j][i] <= 16) {
                                //黒画素の8近傍を探索して1つでも白だったら白にしてループを脱出
                                imgprc2[num][2][j][i] = 235;
                                imgprc2[num][1][j][i] = 128;
                                imgprc2[num][0][j][i] = 128;
                                termination_status = 1;
                                break;
                            } else {
                                imgprc2[num][2][j][i] = imgprc2[num - 1][2][j][i];
                                imgprc2[num][1][j][i] = imgprc2[num - 1][1][j][i];
                                imgprc2[num][0][j][i] = imgprc2[num - 1][0][j][i];
                            }
                        }
                        if(termination_status == 1) {
                            break;
                        }
                    }
                }
            }
        }
    }
    //--膨張--
    /* 例: 元の二値画像はimgprc2[0]に入っている.
     * 例を挙げると: times = 10の時(10回ずつの時)
     * imgprc2[10]を参照してimgprc2[9]に格納する.
     * imgprc2[num + 1]を参照して降順で格納する.
     * 即ちnumのループはtimes - 1(10 - 1 = 9)から0までとなる。
     * 膨張結果の最終格納先はimgprc2[0]である.
     */
    for(int num = times - 1; num >= 0; num--) { //回数ループ
        //最も端の画素は今回は考えない.
        for(int i = 1; i < height - 1; i++) {   //たて
            for(int j = 1; j < width - 1; j++) {    //よこ
                if(mode == 1){
                    //1つ前の結果を参照して処理:4近傍
                    //白画素の4近傍を探索して1つでも黒だったら黒
                    if(imgprc2[num + 1][2][j][i] >= 235) {
                        if(imgprc2[num + 1][2][j+1][i] <= 16 || imgprc2[num + 1][2][j-1][i] <= 16 || imgprc2[num + 1][2][1][i+1] <= 16 || imgprc2[num + 1][2][j][i-1] <= 16) {
                            imgprc2[num][2][j][i] = 16;
                            imgprc2[num][1][j][i] = 128;
                            imgprc2[num][0][j][i] = 128;
                        } else {
                        imgprc2[num][2][j][i] = imgprc2[num + 1][2][j][i];
                        imgprc2[num][1][j][i] = imgprc2[num + 1][1][j][i];
                        imgprc2[num][0][j][i] = imgprc2[num + 1][0][j][i];
                        }
                    } else {
                        imgprc2[num][2][j][i] = imgprc2[num + 1][2][j][i];
                        imgprc2[num][1][j][i] = imgprc2[num + 1][1][j][i];
                        imgprc2[num][0][j][i] = imgprc2[num + 1][0][j][i];
                    }
                } else if(mode == 2) {  //8近傍
                    termination_status = 0;
                    for(int k = -1; k <= 1; k++){   //縦
                        for(int l = -1; l <= 1; l++) {  //横
                            if(l == 0 && k == 0) {
                                //なにもしない
                            } else if(imgprc2[num + 1][2][j + l][i + k] <= 16 && imgprc2[num - 1][2][j][i] == 235) {
                                //白画素の8近傍を探索して1つでも黒だったら黒にしてループを脱出
                                imgprc2[num][2][j][i] = 16;
                                imgprc2[num][1][j][i] = 128;
                                imgprc2[num][0][j][i] = 128;
                                termination_status = 1;
                                break;
                            } else {
                                imgprc2[num][2][j][i] = imgprc2[num + 1][2][j][i];
                                imgprc2[num][1][j][i] = imgprc2[num + 1][1][j][i];
                                imgprc2[num][0][j][i] = imgprc2[num + 1][0][j][i];
                            }
                        }
                        if(termination_status == 1) {
                            break;
                        }
                    }
                }
            }
        }
    }

    //最終オープニング結果prc2[0]からimgoutへ格納.
    for(int i = 0; i < height; i++) {
        for(int j = 0; j < width; j++) {
            for(int k = 0; k < 3; k++) {
                imgout[k][j][i] = imgprc2[0][k][j][i];
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