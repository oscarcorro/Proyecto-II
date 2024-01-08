#include<stdio.h>
#include<string.h>
#include<ctype.h>
#include<stdlib.h>

#include "cabeceras.h"

#define LONGITUD_COMANDO 100

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps);
int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2); 
void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup); 
int BuscaFich(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombre);
void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos);
int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo);
int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre);
int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre,  FILE *fich);
int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino,  FILE *fich);
void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich);
void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich);
void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich);
void GrabarDatos(EXT_DATOS *memdatos, FILE *fich);

int main(){
	char *comando = (char*)malloc(LONGITUD_COMANDO);;
    char *orden = (char*)malloc(LONGITUD_COMANDO);
	char *argumento1 = (char*)malloc(LONGITUD_COMANDO);
	char *argumento2 = (char*)malloc(LONGITUD_COMANDO);
	int i,j;
	unsigned long int m;
    EXT_SIMPLE_SUPERBLOCK *ext_superblock = (EXT_SIMPLE_SUPERBLOCK *)malloc(sizeof(EXT_SIMPLE_SUPERBLOCK));;
    EXT_BYTE_MAPS ext_bytemaps;
    EXT_BLQ_INODOS ext_blq_inodos;
    EXT_ENTRADA_DIR directorio[MAX_FICHEROS];
    EXT_DATOS memdatos[MAX_BLOQUES_DATOS];
    EXT_DATOS datosfich[MAX_BLOQUES_PARTICION];
    int entradadir;
    int grabardatos;
    FILE *fent;
    int salir = 0;
     
    // Lectura del fichero completo de una sola vez
     
    fent = fopen("particion.bin","r+b");
    fread(&datosfich, SIZE_BLOQUE, MAX_BLOQUES_PARTICION, fent);     
     
    memcpy(ext_superblock,(EXT_SIMPLE_SUPERBLOCK *)&datosfich[0], SIZE_BLOQUE);
    memcpy(&directorio,(EXT_ENTRADA_DIR *)&datosfich[3], SIZE_BLOQUE);
    memcpy(&ext_bytemaps,(EXT_BLQ_INODOS *)&datosfich[1], SIZE_BLOQUE);
    memcpy(&ext_blq_inodos,(EXT_BLQ_INODOS *)&datosfich[2], SIZE_BLOQUE);
    memcpy(&memdatos,(EXT_DATOS *)&datosfich[4],MAX_BLOQUES_DATOS*SIZE_BLOQUE);
    
    //Bucle de tratamiento de comandos
    do{
		do{
            printf (">> ");
            fflush(stdin);
            fgets(comando, LONGITUD_COMANDO, stdin);
            //Quito el \n y pongo el el '\0'
            size_t length = strlen(comando);
            if(length > 0 && comando[length - 1] == '\n'){
            comando[length - 1] = '\0';
            }
		}while(ComprobarComando(comando,orden,argumento1,argumento2) != 1); //devuelve 0 si el comando no existe
	    if(strcmp(orden, "info") == 0){
            LeeSuperBloque(ext_superblock);
        }else if(strcmp(orden,"dir") == 0){
            Directorio(directorio, &ext_blq_inodos);
        }else if(strcmp(orden, "bytemaps") == 0){
            Printbytemaps(&ext_bytemaps);
        }else if(strcmp(orden, "rename") == 0){
            Renombrar(directorio, &ext_blq_inodos, argumento1, argumento2);  
        }else if(strcmp(orden, "imprimir") == 0){
            Imprimir(directorio, &ext_blq_inodos, memdatos, argumento1);
        }else if(strcmp(orden, "remove") == 0){
            Borrar(directorio, &ext_blq_inodos, &ext_bytemaps, ext_superblock, argumento1, fent);
        }else if(strcmp(orden, "copy") == 0){
           Copiar(directorio, &ext_blq_inodos, &ext_bytemaps, ext_superblock, memdatos, argumento1, argumento2, fent);
        }
        

        
        // Escritura de metadatos en comandos rename, remove, copy     
        Grabarinodosydirectorio(directorio,&ext_blq_inodos,fent);
        GrabarByteMaps(&ext_bytemaps,fent);
        GrabarSuperBloque(ext_superblock,fent);
        if(grabardatos)
            GrabarDatos(memdatos,fent);
        grabardatos = 0;
        //Si el comando es salir se habrÃ¡n escrito todos los metadatos
        //faltan los datos y cerrar
        if(strcmp(orden,"salir") == 0){
            GrabarDatos(memdatos,fent);
            fclose(fent);
            salir = 1;
            GrabarDatos(memdatos, fent);
        }
    }while(!salir);
}

void Printbytemaps(EXT_BYTE_MAPS *ext_bytemaps){
    //Muestra el contenido del bytemap de inodos
    printf("Inodos: ");
    for(int i = 0; i < MAX_INODOS; i++){
        printf("%d ", ext_bytemaps->bmap_inodos[i]);
    }
    printf("\n");

    //Muestra los primeros 25 elementos del bytemap de bloques
    printf("Bloques [0-25]: ");
    for(int i = 0; i < 25 && i < MAX_BLOQUES_PARTICION; i++){
        printf("%d ", ext_bytemaps->bmap_bloques[i]);
    }
    printf("\n");
}

int ComprobarComando(char *strcomando, char *orden, char *argumento1, char *argumento2){
    int existeComando = 0;
    //Inicializo los argumentos a '\0' porque si se intrduce mal se guardaban ahora se reincian
    *argumento1 = '\0';
    *argumento2 = '\0';

    //Obtenemos orden y los argumentos parseandolos con strtok
    char *token = strtok(strcomando, " ");
    strcpy(orden, token);
    //printf("Orden: %s", orden);
    token = strtok(NULL, " ");
    if(token != NULL){
        strcpy(argumento1, token);
        token = strtok(NULL, " ");
        if(token != NULL){
            strcpy(argumento2, token);
        }
    }
        
    //Comprobamos la existencia del comando introducido
    //printf("\nORDEN: %s, ARG1: %s ARG2: %s\n", orden, argumento1, argumento2);
    if((strcmp(orden, "info") == 0) && (*argumento1 == '\0' ) && (*argumento2 == '\0')){
        //printf("hola");
        existeComando = 1;
    }else if((strcmp(orden, "bytemaps") == 0) && (argumento1[0] == '\0') && (argumento2[0] == '\0')){
        existeComando = 1;
    }else if((strcmp(orden, "dir") == 0) && (argumento1[0] == '\0') && (argumento2[0] == '\0')){
        existeComando = 1;
    }else if((strcmp(orden, "rename") == 0) && (argumento1[0] != '\0') && (argumento2[0] != '\0')){
        existeComando = 1;
    }else if((strcmp(orden, "imprimir") == 0) && (argumento1[0] != '\0') && (argumento2[0] == '\0')){
        existeComando = 1;
    }else if((strcmp(orden, "remove") == 0) && (argumento1[0] != '\0') && (argumento2[0] == '\0')){
        existeComando = 1;
    }else if((strcmp(orden, "copy") == 0) && (argumento1[0] != '\0') && (argumento2[0] != '\0')){
        existeComando = 1;
    }else if((strcmp(orden, "salir") == 0) && (argumento1[0] == '\0') && (argumento2[0] == '\0')){
        existeComando = 1;
    }
    
    if(existeComando == 0){
        printf("ERROR: Comando ilegal [bytemaps,copy,dir,info,imprimir,rename,remove,salir]\n");
    }

    return existeComando;

}

void LeeSuperBloque(EXT_SIMPLE_SUPERBLOCK *psup){

    printf("Bloque: %d", psup->s_block_size);
    printf("\ninodos particion = %d", psup->s_inodes_count);
    printf("\ninodos libres = %d", psup->s_free_inodes_count);
    printf("\nBloques particion = %d", psup->s_blocks_count);
    printf("\nBloques libres = %d", psup->s_free_blocks_count);
    printf("\nPrimer bloque de datos = %d\n", psup->s_first_data_block);

}

void Directorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos){
    for(int i = 1; i < MAX_FICHEROS; i++){//empieza en 1 para no imprimir el directorio raíz
        if(directorio[i].dir_inodo != NULL_INODO){
            printf("%s\tTamaño: %u\tInodo: %hu\t", directorio[i].dir_nfich, inodos->blq_inodos[directorio[i].dir_inodo].size_fichero, directorio[i].dir_inodo);

            //Imprimir bloques que ocupa
            printf("Bloques: ");
            for(int j = 0; j < MAX_NUMS_BLOQUE_INODO && inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j] != NULL_BLOQUE; j++){
                printf("%d ", inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j]);
            }

            printf("\n"); 
        }
    }
}

int Renombrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, char *nombreantiguo, char *nombrenuevo){
    int exito = 0;
    int existeFicheroAntiguo = 0;
    int existeFicheroNuevo = 0;
    int indiceFicheroAntiguo = -1;

    //Buscamos el nombreantiguo en el directorio
    for(int i = 1; i < MAX_FICHEROS; i++){
        if(strcmp(directorio[i].dir_nfich, nombreantiguo) == 0){
            existeFicheroAntiguo = 1;
            indiceFicheroAntiguo = i;
        }
    }

    if(existeFicheroAntiguo){
        //Comprobamos si existe el nombre nuevo en el directorio
        for(int i = 0; i < MAX_FICHEROS; i++){
            if(strcmp(directorio[i].dir_nfich, nombrenuevo) == 0){
                existeFicheroNuevo = 1;
                printf("ERROR: El fichero %s ya existe\n", nombrenuevo);
            }
        }
    
        //Deberíamos actualizar el nombre en la estructura de inodos pero como no tiene esta variable lo hacemos solo en el dir
        if(!existeFicheroNuevo){
            //Si existe el antiguo y no el nuevo se acutaliza el nombre
            strcpy(directorio[indiceFicheroAntiguo].dir_nfich, nombrenuevo);
            exito = 1;
        }

    }else{
        printf("ERROR: Fichero %s no encontrado\n", nombreantiguo);
    }

    return exito;
}

int Imprimir(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_DATOS *memdatos, char *nombre){

    int encontrado = 0;

    //Comprobamos si existe el archivo en el directorio
    for(int i = 1; i < MAX_FICHEROS; i++){
        if(strcmp(directorio[i].dir_nfich, nombre) == 0){
            encontrado = 1;
            //Obtenemos el inodo asociado al archivo
            EXT_SIMPLE_INODE *inodo = &(inodos->blq_inodos[directorio[i].dir_inodo]);

            //Imprimimos el contenido del archivo bloque a bloque
            for(int i = 0; i < MAX_NUMS_BLOQUE_INODO; i++){
                if (inodo->i_nbloque[i] != 0xFFFF) {
                    for(int j = 0; j < SIZE_BLOQUE; j++){
                        printf("%c", memdatos[inodo->i_nbloque[i] - 4].dato[j]);
                    }
                }
            }
        }
        
    }

    //Si no se encuentra el archivo error
    if(!encontrado){
        printf("ERROR: Fichero %s no encontrado\n", nombre);
    }

    return encontrado;//devolvemos un int porque la función lo pide pero no es necesario
}

int Borrar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, char *nombre,  FILE *fich){

    int existeFichero = 0;
    //Comprobamos que el fichero existe en el directorio
    for(int i = 1; i < MAX_FICHEROS; i++){
        if(strcmp(directorio[i].dir_nfich, nombre) == 0){
            existeFichero = 1;
            inodos->blq_inodos[directorio[i].dir_inodo].size_fichero = 0;//Tamaño del fichero a 0
            for(int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++) {
                int numBloque = inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j];
                if(numBloque != NULL_BLOQUE){
                    //Marcamos el bloque como libre en el byte map de bloques
                    ext_bytemaps->bmap_bloques[numBloque] = 0;
                    //Incrementamos el número de bloques vacios
                    ext_superblock->s_free_blocks_count++;
                }
                //Libreamos el bloque en el inodo
                inodos->blq_inodos[directorio[i].dir_inodo].i_nbloque[j] = NULL_BLOQUE;
            }
            
            //Libreamos el inodo e incrementamos el número de inodos vacíos
            ext_bytemaps->bmap_inodos[directorio[i].dir_inodo] = 0;
            ext_superblock->s_free_inodes_count++;
            //Eliminamos el nombre del directorio
            strcpy(directorio[i].dir_nfich, "");
            //Ponemos el número de inodo a null
            directorio[i].dir_inodo = NULL_INODO;
            
        }
    }

    if(!existeFichero){
        printf("ERROR: Fichero %s no encontrado\n", nombre);
    }

    //Actualizamos los cambios en el archivo partición
    Grabarinodosydirectorio(directorio, inodos, fich);
    GrabarByteMaps(ext_bytemaps, fich);
    GrabarSuperBloque(ext_superblock, fich);

    return 1;
}

int Copiar(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, EXT_BYTE_MAPS *ext_bytemaps, EXT_SIMPLE_SUPERBLOCK *ext_superblock, EXT_DATOS *memdatos, char *nombreorigen, char *nombredestino, FILE *fich) {
    
    int inodo_libre = -1;

    //buscamos un inodo libre
    for(int i = 2; i < MAX_INODOS; i++){
        if (ext_bytemaps->bmap_inodos[i] == 0) {
            inodo_libre = i;
            break;
        }
    }

    //si no hay inodos libres se muestra el error
    if(inodo_libre == -1){
        printf("No hay inodos libres.\n");
        return 0;
    }

    //buscamos el inodo del fichero origen para poder copiar
    int inodo_origen = -1;
    for(int i = 0; i < MAX_FICHEROS; i++){
        if(strcmp(directorio[i].dir_nfich, nombreorigen) == 0){
            inodo_origen = directorio[i].dir_inodo;
            break;
        }
    }

    if(inodo_origen == -1){
        printf("Fichero origen no encontrado.\n");
        return 0;
    }

    //copiamos el tamaño del fichero al nuevo inodo
    inodos->blq_inodos[inodo_libre].size_fichero = inodos->blq_inodos[inodo_origen].size_fichero;
    // Marcamos el nuevo inodo como ocupado
    ext_bytemaps->bmap_inodos[inodo_libre] = 1;
    ext_superblock->s_free_inodes_count--;

    // Buscamos bloques libres para asignar al nuevo inodo
    int bloque_libre[MAX_NUMS_BLOQUE_INODO];
    for(int j = 0; j < MAX_NUMS_BLOQUE_INODO; j++){
        int bloque_origen = inodos->blq_inodos[inodo_origen].i_nbloque[j];
        if(bloque_origen != NULL_BLOQUE){
            //buscamos un bloque libre en el bytemap de bloques
            bloque_libre[j] = -1;
            for(int k = PRIM_BLOQUE_DATOS; k < MAX_BLOQUES_DATOS; k++){
                if(ext_bytemaps->bmap_bloques[k] == 0){
                    bloque_libre[j] = k;
                    ext_bytemaps->bmap_bloques[k] = 1;
                    ext_superblock->s_free_blocks_count--;
                    break;
                }
            }

            //imprimimos error si no quedan bloques libres
            if(bloque_libre[j] == -1){
                printf("No hay bloques libres.\n");
                return 0;
            }

            //copiamos el contenido del bloque del archivo original al nuevo bloque
            memcpy(memdatos[bloque_libre[j]-4].dato, memdatos[bloque_origen-4].dato, SIZE_BLOQUE);
            //Asignamos el nuevo bloque al nuevo inodo
            inodos->blq_inodos[inodo_libre].i_nbloque[j] = bloque_libre[j];
        }
    }

    //creamos una nueva entrada en el directorio para el fichero destino
    for(int i = 1; i < MAX_FICHEROS; i++){
        if (directorio[i].dir_inodo == NULL_INODO) {
            strcpy(directorio[i].dir_nfich, nombredestino);
            directorio[i].dir_inodo = inodo_libre;
            break;
        }
    }

    //finalmente actualizamos la información en el archivo partición para que se sincronice
    Grabarinodosydirectorio(directorio, inodos, fich);
    GrabarByteMaps(ext_bytemaps, fich);
    GrabarSuperBloque(ext_superblock, fich);

    return 1;
}


void Grabarinodosydirectorio(EXT_ENTRADA_DIR *directorio, EXT_BLQ_INODOS *inodos, FILE *fich) {
    fseek(fich, SIZE_BLOQUE * 3, SEEK_SET); 
    fwrite(directorio, SIZE_BLOQUE, 1, fich);
    fseek(fich, SIZE_BLOQUE * 2, SEEK_SET); 
    fwrite(inodos, SIZE_BLOQUE, 1, fich);
}

void GrabarByteMaps(EXT_BYTE_MAPS *ext_bytemaps, FILE *fich) {
    fseek(fich, SIZE_BLOQUE, SEEK_SET); 
    fwrite(ext_bytemaps, SIZE_BLOQUE, 1, fich);
}

void GrabarSuperBloque(EXT_SIMPLE_SUPERBLOCK *ext_superblock, FILE *fich) {
    fseek(fich, 0, SEEK_SET); 
    fwrite(ext_superblock, SIZE_BLOQUE, 1, fich);
}

void GrabarDatos(EXT_DATOS *memdatos, FILE *fich) {
    fseek(fich, SIZE_BLOQUE * 4, SEEK_SET); 
    fwrite(memdatos, SIZE_BLOQUE, MAX_BLOQUES_DATOS, fich);
}


