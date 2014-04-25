#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <regex.h>


/* DONE:
Recherche récursive d'un fichier grace à son nom:
Intégration des options
Fonction d'affichage des infos d'un fichier (utiliser struct stat):
Gerer les options: le répertoire courant
*/

/* TODO LIST:
Intégrer les regex
*/

int trouve = 0; // variable globale mise à 1 si on trouve le fichier dans l'arborescence qui descend du path de recherche

typedef struct options{ // regroupe les options pour réduire le nombre de paramètres
    short last_modif;
    short taille;
    short type;
    short protection;
    short date_creation;
}options;

void init_options(options *opt){ //Initialise les champs d'une struct options
                    opt->date_creation = 0;
                    opt->last_modif = 0;
                    opt->taille = 0;
                    opt->type = 0;
                    opt->protection = 0;
}

void show_help(){ //Affiche l'aide sur la commande search
    
    printf("              --------------------- \n");
    printf("\n\nsearch est une commande qui permet de rechercher récursivement des fichiers selon leur nom");
    printf("\n\n search [Nom_du_Répertoire] [-option] Nom_du_Fichier");
    printf("\n\n Liste des options:");
    printf("\n -d : Afficher la date de création et dernière utilisation du fichier");
    printf("\n -m : Afficher la date de la dernière modification du fichier");
    printf("\n -d : Afficher la taille du fichier");
    printf("\n -t : Afficher le type du fichier");
    printf("\n -p : Afficher la protection du fichier");
    printf("\n -a : Afficher toutes les caracteristiques ci dessus");
    printf("\n -n : Parcourir n niveaux, avec n un entier positif\n\n");
    printf("                --------------------- \n");
}

char type_fichier(struct stat status) {
        // Retourne un caractère suivant le type du fichier
    if (S_ISREG(status.st_mode)) return '-';
    if (S_ISFIFO(status.st_mode)) return 'p';
    if (S_ISCHR(status.st_mode)) return 'c';
    if (S_ISBLK(status.st_mode)) return 'b';
    if (S_ISDIR(status.st_mode)) return 'd';
    if (S_ISLNK(status.st_mode)) return 'l';
    if (S_ISSOCK(status.st_mode)) return 's';
}

int is_num(char c){
    return ((c > 47) && (c < 58));
}

/* FONCTION D'AFFICHAGE TODO*/
// droits,
void afficher_infos(const char * filename, struct stat status, options opt ){
    char* access = NULL;
    char* creation = NULL;
    char* modification = NULL;
    if (opt.type){
        printf("%c", type_fichier(status));
    }
    if (opt.protection){
        char droits[]="---------";
        if (status.st_mode & S_IRUSR)// GRP OTH
            droits[0] = 'r';
        if (status.st_mode & S_IWUSR)
            droits[1] = 'w';
        if (status.st_mode & S_IXUSR)
            droits[2] = 'x';
        if (status.st_mode & S_IRGRP)// GRP OTH
            droits[3] = 'r';
        if (status.st_mode & S_IWGRP)
            droits[4] = 'w';
        if (status.st_mode & S_IXGRP)
            droits[5] = 'x';
        if (status.st_mode & S_IROTH)// GRP OTH
            droits[6] = 'r';
        if (status.st_mode & S_IWOTH)
            droits[7] = 'w';
        if (status.st_mode & S_IXOTH)
            droits[9] = 'x';
        printf("%s ", droits);
    }
    if (opt.taille)
        printf("%d ", (int)status.st_size);
    if (opt.date_creation){
        access = ctime(&status.st_atime);
        creation = ctime(&status.st_ctime);
        access[strlen(access) - 1] = '\0';
        creation[strlen(creation) - 1] = '\0';
        printf("%s %s ", creation, access);
    }
    if (opt.last_modif){
        modification = ctime(&status.st_mtime);
        modification[strlen(modification)-1] = '\0';
        printf("%s ", modification);
    }
    printf("%s\n", filename);


}


short get_option(char* option, options *opt, int *profondeur){
    int i = 1, k = 0;
    char str_profondeur[10];

    if (option== NULL) //
            return 1; //
        else if (!strcmp(option, "--help")){
            show_help();
        }
        else if (option[0] != '-')
                return 0;
            else{ //
                    if (strstr(option, "a") != NULL){
                        opt->date_creation = 1;
                        opt->last_modif = 1;
                        opt->taille = 1;
                        opt->type = 1;
                        opt->protection = 1;
                    }
                    else{
                        if (strstr(option, "d") != NULL)
                            opt->date_creation = 1;
                        if (strstr(option, "m") != NULL)
                            opt->last_modif = 1;
                        if (strstr(option, "s") != NULL)
                            opt->taille = 1;
                        if (strstr(option, "t") != NULL)
                            opt->type = 1;
                        if (strstr(option, "p") != NULL)
                            opt->protection = 1;
                    }
            }
        if (!is_num(option[1]))
            return 1;
        while (option[i] != '\0'){

            if (!is_num(option[i]))
                return 0;
            else
                str_profondeur[k++] = option[i++];
        }
            str_profondeur[k] = '\0';
        *profondeur = atoi(str_profondeur);
        return 1;
}

int matches(const char *str, regex_t regex){
    return  !regexec (&regex, str, 0, NULL, 0);
}

void search(const char * path, regex_t nom_fichier, options opt, int nb_niveaux){
// Rôle : traverse le répertoire et, éventuellement, ses sous-répertoires à la recherche du fichier de nom nom_fichier, avec des options
        if (nb_niveaux < 1)
            return;
        // --------- déclaration des variables ------------//
        struct dirent * entry = NULL; //simule un fichier lu ('dirent' pour DIRectory ENTity)
        struct stat status; // structure utilisée pour récupérer les informations stockées dans un inode
        char filename [256]; // sert de path absolu pour explorer les fichiers du répertoire
        DIR *dir = NULL; //simule un répertoire
        
        // ----------- phase de recherche -------- //
        if ( (dir = opendir(path)) == NULL){
            return;
        }

        while ((entry = readdir(dir)) != NULL) { //readdir(*DIR) retourne un 'dirent' s'il y a encore des fichiers à lire, NULL sinon
            if (entry -> d_type ==  DT_LNK)
                return;
            sprintf(filename, "%s/%s", path, entry -> d_name); // donne 'path'/'nom_du_f'
            stat(filename, &status); // mettre les infos du fichier de path absolu filename dans la struct status
            /* cas d'un répertoire */
            
            if ( S_ISDIR(status.st_mode)) {
                
                if (strcmp(entry -> d_name, ".") && strcmp(entry -> d_name, "..")) // on doit filtrer . et .. !
                    // recursivité FTW ! *_*
                    search(filename, nom_fichier, opt, nb_niveaux - 1);
                }

            /* cas d'un fichier */
            else {
                    // On récupère le nom du fichier avec entry->d_name et on compare avec le nom du fichier qu'on cherche
                    if (matches(entry->d_name, nom_fichier)){ // si nom_fichier == le fichier courant
                        if (!trouve) printf("Le fichier existe au(x) chemin(s) suivant(s): \n"); // affichée au max une fois
                        afficher_infos(filename, status, opt);
                        //printf("%s %c %s \n", ctime(&status.st_mtime),type_fichier(status), path);
                        //printf("%s", afficher_infos(path, status, date_creation, last_modif, taille, type, protection));
                        trouve = 1;
                    }
                continue;
                }

        }

        closedir(dir); // fermer le répertoire
    }


void main(int argc, char** argv){
    int i = 1;
    int profondeur = (int) INFINITY;
    options opt;
    char path [256];
    regex_t regex ;
    
    trouve = 0;
    if ((argc == 2) || argv[1][0] == '-'){
        getcwd(path, sizeof(path));
        i = 0;   
    }
    else{
        sprintf(path , "%s", argv[1]);
    }
    printf("%s\n", path);
    init_options(&opt);
    while (i < argc-1){
        get_option(argv[i], &opt, &profondeur);
        i++;
    }
    if (opendir(path) == NULL){
        printf("Le chemin que vous avez entré n'existe pas\n");
        return;
    }
    regcomp (&regex, argv[argc-1], REG_NOSUB | REG_EXTENDED);
    search(path, regex, opt, profondeur); //respectivement : path nom_fichier options
    
    if (!trouve)
        printf("Le fichier que vous avez recherché n'existe pas au path spécifié\n");
}