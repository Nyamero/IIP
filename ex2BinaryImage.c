#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

//�F��ԕϊ��̃}�N����`
#define CH 3
#define Ych 0

#define ROW 3
#define COL 3

//��l�摜�����̃u���b�N�T�C�Y��`
#define FSIZE 7   //�t�B���^�T�C�Y
#define BSIZE 35   //�u���b�N�T�C�Y


/*=====================*
 *�v���g�^�C�v�錾      *
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
 *�O���[�o���ϐ�        *
 *=====================**/
//�w�b�_�[���e
unsigned char header[54];

//�摜�f�[�^�i�[�z��
unsigned char imgin[3][512][512];
//�摜�f�[�^�i�[�z��2
unsigned char imgprc[3][512][512];
//�摜�f�[�^�R�s�[�i�[�z��
unsigned char imgout[3][512][512];

//�w�b�_�[�e����̊i�[
int filesize;
int offset;
int width;
int height;
int bits;
int inserted_bits;


//�F��ԕϊ��̏��i�[

//RGB����YCbCr�ւ̕ϊ��s��
double matrix_rgb_to_ybr[ROW][COL] =
{
    { 0.2990, 0.5870, 0.1140},
    {-0.1687,-0.3313, 0.5000},
    { 0.5000,-0.4187,-0.0813}
};

//YCbCr����RGB�ւ̕ϊ��s��
double matrix_ybr_to_rgb[ROW][COL] =
{
    { 1.0000, 0.0000, 1.4020},
    { 1.0000,-0.3441,-0.7141},
    { 1.0000, 1.7720, 0.0000}
};

int rgb_in[CH]; // ����RGB�M��(�����l)
int ybr[CH]; // YCbCr�M��(�����l)
int rgb_out[CH]; // �o��RGB�M��(�����l)
double dtemp[CH];   //YCbCr�M���ւ̕ύX���
int ch;
int itemp;

/**
 * main�֐�: �e�֐��̌Ăяo��
*/
int main(int argc, char *argv[]){
  
    get_data(argc, argv);

    conversion_ybr();

    processing();   //�又��
    
    ybr_to_rgb();

    put_data(argc, argv);

    return 0;
}

/**
 * �摜�f�[�^�擾
*/
void get_data(int argc, char *argv[]){

    FILE *fp;

    char filename[256];

    //�R�}���h���C������������̓t�@�C�������擾
    strcpy(filename, argv[1]);
    printf("���̓t�@�C����: %s\n", filename);

    fp = fopen(filename, "rb");
    if(fp == NULL){
        printf("%s���I�[�v���ł��܂���. \n", filename);
        exit(1);
    }

    printf("%s���I�[�v�����܂���. \n", filename);

    //�z��Ƀw�b�_���e��S�Ċi�[
    for(int i = 0; i < 54; i++){
        header[i] = getc(fp);
    }
        
    //---�t�@�C���T�C�Y---
    filesize = show_data(2, 5, 1);


    //---�I�t�Z�b�g---
    offset = show_data(10, 13, 1);

        
    //---�摜�̕�---
    width = show_data(18, 21, 1);
    printf("width --- %d ---\n", width);

    //---�摜�̍���---
    height = show_data(22, 25, 1);
    printf("height --- %d ---\n", height);

    //---�P��f������̃r�b�g��---
    bits = show_data(28, 29, 1);

    //---�}���r�b�g��---
    inserted_bits = filesize - offset - width * height * (bits/8);
        
    /**
     * �摜�f�[�^���̓ǂݍ���
     */
    for(int i = height - 1; i >= 0; i--){   //�c���[�v, �ォ�牺
        for(int j = 0; j < width; j++){ //�����[�v, ������E(+)
            for(int k = 0; k < 3; k++){ //3�F��, B G R(+)
                imgin[k][j][i] = getc(fp);
            }
        }
    }

    fclose(fp);
    printf("%s���N���[�Y���܂���.\n\n", filename);
}


/**
 * get_data���ł̎w�肪�������ꍇ�̃f�[�^�\��
*/
int show_data(int start, int end, int status){
    int result = 0;
    if(status == 1){ //���̌�̌v�Z���K�v�ȏꍇ
        result = calc_data(start, end);
    }
    return result;
}


/**
 * �}���r�b�g���̌v�Z
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
 * ��l�摜��������
*/
void processing(void){

    double tmp;    //�Վ��i�[�p

    double filter_gaussian[FSIZE][FSIZE];

    double pi = 4.0 * atan(1.0);    //��=��3.14...�̋ߎ�
    double sigma = 1.7; //�Ђ̒�`

    double calc_tmp = (1 / (2 * pi * pow(sigma, 2)));

    printf("calc_tmp = %f\n", calc_tmp);

    /*�K�E�V�A���t�B���^����(BSIZE*BSIZE)*/
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
                    //�̈�O(�v���X�摜�ɃI�[�o�[)�ł���Ƃ�[�����ϊ�]
                    if((j - (int)(FSIZE / 2) + t) >= width || (i - (int)(FSIZE / 2) + t) >= height){   //�̈�O(�v���X�����ɃI�[�o�[)�ł���Ƃ�
                        tmp += (double)imgin[2][abs(j - (int)(FSIZE / 2) + t) - (abs(j - (int)(FSIZE / 2) + t) - width)][abs(i - (int)(FSIZE / 2) + s) - (abs(i - (int)(FSIZE / 2) + s) - height)] * filter_gaussian[t][s];
                    } else {        //�̈���@�������́@�̈�O(�}�C�i�X�����ɃI�[�o�[)�ł���Ƃ�
                        tmp += (double)imgin[2][abs(j - (int)(FSIZE / 2) + t)][abs(i - (int)(FSIZE / 2) + s)] * filter_gaussian[t][s];
                    }
                }
            } 

            imgprc[2][j][i] = (int)round(tmp);
            imgprc[1][j][i] = 128;
            imgprc[0][j][i] = 128;
            
        }
    }

    //�������Ƃ�
    /*
    for(int i = 0; i < height; i++){
        for(int j = 0; j < width; j++){
            imgout[2][j][i] = imgprc[2][j][i] - imgin[2][j][i];
            imgout[1][j][i] = imgprc[1][j][i];
            imgout[0][j][i] = imgprc[0][j][i];
        }
    }
    */

    //��l
    double threshold;
    
    //�S��f����
    for(int i = 0; i < height; i++){    //�c���[�v
        for(int j = 0; j < width; j++){ //�����[�v
            //臒l��������
            threshold = 0;
            
            //�u���b�N�Q��
            //���Ԃɑ��... �Ώۉ�f�𒆐S�Ɏ���BSIZE�����i�[
            for(int k = 0; k < BSIZE; k++) {     //�c���[�v
                for(int l = 0; l < BSIZE; l++) { //�����[�v
                    //�̈�O(�v���X�摜�ɃI�[�o�[)�ł���Ƃ�[�����ϊ�]
                    if(j - (int)(BSIZE / 2) + l >= width || i - (int)(BSIZE / 2) + k >= height) {
                        threshold += imgprc[2][abs(j - (int)(BSIZE / 2) + l) - (abs(j - (int)(BSIZE / 2) + l) - width)][abs(i - (int)(BSIZE / 2) + k) - (abs(i - (int)(BSIZE / 2) + k) - height)];
                    } else {    //�̈�� �������� �̈�O(�}�C�i�X�����ɃI�[�o�[)�ł���Ƃ�[�����ϊ�]
                        threshold += imgprc[2][j - (int)(BSIZE / 2) + l][i - (int)(BSIZE / 2) + k];
                    }
                }
            }
            //���ς��Ƃ�
            threshold /= pow(BSIZE, 2);

            if(j == 5) printf("th: %f, img: %d\n", threshold, imgprc[2][j][i]);

            if(threshold < imgin[2][j][i]) {   //���ς�����菬�����Ƃ�(<): ��
                imgout[2][j][i] = 235;
                imgout[1][j][i] = 128;
                imgout[0][j][i] = 128;
            } else {                           //���ς����Ƃ��傫���Ƃ�(<): ��
                imgout[2][j][i] = 16;
                imgout[1][j][i] = 128;
                imgout[0][j][i] = 128;
            }
        }
    }


}


/**
 * �摜�̏o��
*/
void put_data(int argc, char *argv[]){
    FILE *fp;
    char filename[256];
    strcpy(filename, argv[2]);

    printf("�o�̓t�@�C����: %s\n", filename);
        
    fp = fopen(filename, "wb");
    printf("%s���I�[�v�����܂���.\n", filename);

    //�w�b�_�[�����i�[
    for(int i = 0; i < 54; i++){
        fputc(header[i], fp);
    }

    //�摜�����i�[
    for(int i = height - 1; i >= 0; i--){
        for(int j = 0; j < width; j++){
            for(int k = 0; k < 3; k++){
                fputc(imgout[k][j][i], fp);
            }
        }
    }

    //4�o�C�g�A���C�������g�ɕK�v�ȃr�b�g��}��
    for(int i = 0; i < inserted_bits; i++){
        fputc('\0', fp);
    }

    fclose(fp);
    printf("%s���N���[�Y���܂���.\n", filename);
}

/**
 * [�F��ԕϊ�][�Œ�l�ւ̒u��]
 * YCbCr�M���̕ϊ��l�����
*/
void conversion_ybr(void){

    int mode_y = 1;     //y�̕ϊ��X�e�[�^�X
    int mode_Cb = 0;    //Cb�̕ϊ��X�e�[�^�X
    int mode_Cr = 0;    //Cr�̕ϊ��X�e�[�^�X


    /*int rgb_in[3];�ւ̓ǂݍ���
     *  rgb_in[0]: R
     *  rgb_in[1]: G
     *  rgb_in[2]: B
     */

    for(int i = 0; i < height; i++){    //�c���[�v, �������(+)
        for(int j = 0; j < width; j++){ //�����[�v, ������E(+)
            for(int k = 2; k >= 0; k--){//�F���[�v, R G B(-)
                rgb_in[2 - k] = imgin[k][j][i]; //[�i�[]rgb_in��0~2��RGB imgin��0~2��CrCbY
            }

                

            //--- RGB�M��(�����l)����YCbCr�M��(�����l)�ւ̕ϊ� ---
            for(ch = 0; ch < CH; ch++){
                dtemp[ch] = 0.0;
                for (int i = 0; i < COL; i++){
                    dtemp[ch] += matrix_rgb_to_ybr[ch][i] * (double)rgb_in[i];
                }
            }
                
                
            //--- YCbCr�M��(�����l)����YCbCr�M��(�����l)�ւ̕ϊ� ---
            for (ch = 0; ch < CH; ch++){
            // �l�̌ܓ�
                if (dtemp[ch] > 0.0){
                    itemp = (int)(dtemp[ch] + 0.5);
                } else {
                    itemp = (int)(dtemp[ch] - 0.5);
                }

                // Cb,Cr�M���ɃI�t�Z�b�g�l128��������
                if (ch != Ych){
                    itemp += 128;
                }

                // �M���l��0~255�͈͓̔��ɐ�������
                if (itemp > 255){
                    itemp = 255;
                } else if (itemp < 0){
                    itemp = 0;
                }
                /* YCbCr�M���l(�����l)���i�[
                * �e�p�^�[���ɂ����ē����ύX:
                * ch == 0(Y) ���� mode_y == 0�̏ꍇ(�Œ�l) 128�ɌŒ�
                * ...�ȉ����l
                */
                if((mode_y == 0 && ch == 0) || (mode_Cb == 0 && ch == 1) || (mode_Cr == 0 && ch == 2)){
                    ybr[ch] = 128;
                } else {
                    ybr[ch] = itemp;
                }
            }
                

            //�z��int imgin[3][512][512]�ւ̊i�[
            for(int k = 2; k >= 0; k--){
                imgin[k][j][i] = ybr[2 - k];
            }

        }
    }

}

/**
 * [�F��ԕϊ�]
 * YCbCr����RGB�ւ̕ϊ�
 * �e��ϐ��̓J�E���^�[�ϐ��ȊO�O���[�o���ϐ��Ƃ��Đ錾
*/
void ybr_to_rgb(void){
    

    /*int rgb_out[3];�ւ̓ǂݍ���
        rgb_in[0]: R
        rgb_in[1]: G
        rgb_in[2]: B
    */


    for(int i = 0; i < height; i++){    //�c���[�v, �������(+)
        for(int j = 0; j < width; j++){ //�c���[�v, ������E(+)
            for(int k = 2; k >= 0; k--){//�F���[�v, BGR(-)
                ybr[2 - k] = imgout[k][j][i]; //[�i�[]ybr��0~2��RGB imgin��0~2��BGR
            }            
            //--- YCbCr�M��(�����l)����RGB�M��(�����l)�ւ̕ϊ� ---
            for (ch = 0; ch < CH; ch++){
                dtemp[ch] = 0.0;
                for (int l = 0; l < COL; l++){
                    if (l == Ych){
                        dtemp[ch] += matrix_ybr_to_rgb[ch][l] * (double)ybr[l];
                    }else {// Cb,Cr�M����,�I�t�Z�b�g�l128��������
                        dtemp[ch] += matrix_ybr_to_rgb[ch][l] * (double)(ybr[l] - 128);
                    }
                }
            }

            //--- RGB�M��(�����l)����RGB�M��(�����l)�ւ̕ϊ� ---
            for (ch = 0; ch < CH; ch++){
                // �l�̌ܓ�
                if (dtemp[ch] > 0.0){
                    itemp = (int)(dtemp[ch] + 0.5);
                } else {
                    itemp = (int)(dtemp[ch] - 0.5);
                }
                // �M���l��0~255�͈͓̔��ɐ�������
                if (itemp > 255){
                    itemp = 255;
                } else if (itemp < 0){
                    itemp = 0;
                }
                // RGB�M���l(�����l)���i�[
                rgb_out[ch] = itemp;
            }

            //�z��int imgout[3][512][512]�ւ̊i�[
            for(int k = 2; k >= 0; k--){
                imgout[k][j][i] = rgb_out[2 - k];
            }
        }
    }
}