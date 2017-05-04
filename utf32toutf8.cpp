#include <stdio.h>
#include <stdlib.h>


// UTF-32 >> UTF-8変換：一文字 (bLE=true ... Little Endian)
int utf32_to_utf8_char (unsigned char* utf32, unsigned char* utf8, bool bLE = true)
{
	unsigned long utf32_l = 0;

	// UTF32のバラバラのbyteをつなげて数値に変換（エンディアン考慮）
	if (bLE) {
		utf32_l = (utf32[3] << 24) | (utf32[2] << 16) |
			  (utf32[1] <<  8) | (utf32[0]);
	} else {
		utf32_l = (utf32[0] << 24) | (utf32[1] << 16) |
			  (utf32[2] <<  8) | (utf32[3]);
	}

	// 値の範囲によって分岐しUTF-8へ変換
	if (utf32_l < 0x80) {
		utf8[0] = utf32_l & 0x7f;
		return 1;
	}
	else if (utf32_l < 0x800) {
		utf8[0] = (0x3 << 6) |  (utf32_l >>  6);
		utf8[1] = (0x1 << 7) | ((utf32_l >>  0) & 0x3f);
		return 2;
	}
	else if (utf32_l < 0x10000) {
		utf8[0] = (0x7 << 5) |  (utf32_l >> 12);
		utf8[1] = (0x1 << 7) | ((utf32_l >>  6) & 0x3f);
		utf8[2] = (0x1 << 7) | ((utf32_l >>  0) & 0x3f);
		return 3;
	}
	else if (utf32_l < 0x10ffff) {
		utf8[0] = (0xf << 4) |  (utf32_l >> 18);
		utf8[1] = (0x1 << 7) | ((utf32_l >> 12) & 0x3f);
		utf8[2] = (0x1 << 7) | ((utf32_l >>  6) & 0x3f);
		utf8[3] = (0x1 << 7) | ((utf32_l >>  0) & 0x3f);
		return 4;
	}

	// 当てはまらない場合は変換せずスキップ
	return 0;
}


// UTF-32 >> UTF-8変換：テキスト文字列 (bLE=true ... Little Endian)
int utf32_to_utf8 (char* utf32, char* utf8, bool bLE = true)
{
	int from_i = 0;
	int to_i = 0;

	// 4byteずつ進めながら文字列を変換（末尾0x0 x 4byteでループ脱出）
	while (true) {
		if ((utf32[from_i + 0] == 0x0) && (utf32[from_i + 1] == 0x0) &&
		    (utf32[from_i + 2] == 0x0) && (utf32[from_i + 3] == 0x0)) break;
		int inc = utf32_to_utf8_char(
			(unsigned char*)(utf32 + from_i), 
			(unsigned char*)(utf8 + to_i), bLE);
		from_i += 4;
		to_i += inc;	// UTF-8は変換結果のbyte数分ポインタを進める
	}
	utf8[to_i++] = 0x0;

	return to_i;
}


void test1()
{
        wchar_t input[] = L"亜あいうえおA";     // UTF-32LE
        char output[256];                       // UTF-8

        int len = utf32_to_utf8 ((char*)input, output);

        int i=0;
        char* tmp = (char*)input;
        printf("UTF-32: ");
        while(tmp[i]!=0x0 || tmp[i+1]!=0x0 || tmp[i+2]!=0x0 || tmp[i+3]!=0x0) {
                printf("%02x ", (unsigned char)(tmp[i++]));
        }
        printf("\n\nUTF-8: ");
        i=0;
        while(output[i] != 0x0) {
                printf("%02x ", (unsigned char)(output[i++]));
        }
        printf("\n\n");
        printf("E4BA9C = 亜\n");
        printf("E38182 = あ\n");
        printf("E38184 = い\n");
        printf("E38186 = う\n");
        printf("E38188 = え\n");
        printf("E3818A = お\n");
        printf("41 = A\n");
}


// input_pathのUTF-32LEファイルをoutput_pathにUTF-8で出力
bool convert_file(char* input_path, char* output_path)
{
	FILE * ifp;
	FILE * ofp;

	if ((ifp=fopen(input_path, "rb")) == NULL) {
		printf("input file path error.\n");
		return false;
	}
	if ((ofp=fopen(output_path, "w")) == NULL) {
		fclose(ifp);
		printf("output file path error.\n");
		return false;
	}

        char output[61];                // UTF-8(max 6bytes) * 10 char + NULL
	char input_buff[44];		// UTF-32(4bytes) * 10 char + NULL

	// UTF-32LEファイルを40バイトずつ読み取ってUTF-8に変換 >> 出力
	while(true) {
		size_t size = fread(input_buff, sizeof(char), 40, ifp); 
		input_buff[size+0] = 0x0;
		input_buff[size+1] = 0x0;
		input_buff[size+2] = 0x0;
		input_buff[size+3] = 0x0;

        	int len = utf32_to_utf8 ((char*)input_buff, output, true);
		fprintf(ofp, "%s", output);

		if (size != 40) break;
	}

	fclose(ofp);
	fclose(ifp);

	return true;
} 

int main(int argc, char **argv)
{
	if (argc != 3) {
		printf("usage: a.out [input-file-path(UTF-32LE)] [output-file-path(UTF-8)]\n");
		return 1;
	}

	if (convert_file(argv[1], argv[2])) {
		printf("conversion completed.\n");
		return 0;
	} else {
		printf("convert error.\n");
		return 1;
	}
}
