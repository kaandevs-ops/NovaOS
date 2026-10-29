#include "../../src/runtime/nova_desktop_open.h"
#include "nova_files_ui.h"
#include "nova_files_font.h"
#include "../../src/runtime/nova_ui_permissions.h"
#include "../../src/runtime/nova_ui_font.h"
#define C_BG 0x00fdfdfbU
#define C_SIDEBAR 0x00edf1f0U
/* Shared geometry drives both painting and input; authority remains in the core. */
static unsigned int visible_rows(const struct nova_files_ui*u){
 unsigned int h=u->viewport_height>=400U?u->viewport_height:400U;
 unsigned int n=(h-160U)/NOVA_FILES_UI_ROW_H;return n>NOVA_FILES_MAX_ENTRIES?NOVA_FILES_MAX_ENTRIES:n;
}
/* View indices never reorder or expand the bounded read-only model. */
static int entry_name_order(const char*a,const char*b){unsigned int i=0;while(a[i]&&a[i]==b[i])i++;return (unsigned char)a[i]-(unsigned char)b[i];}
/* ASCII case folding only; UTF-8 filenames remain intact. No filesystem
 * operation or recursive search is performed by this bounded view filter. */
static unsigned char filter_fold(unsigned char c){return c>='A'&&c<='Z'?(unsigned char)(c+32):c;}
static int filter_matches(const char *name,const char *query){
 if(!query[0])return 1;
 for(unsigned int start=0;name[start];start++){
  unsigned int i=0;while(query[i]&&name[start+i]&&filter_fold((unsigned char)name[start+i])==filter_fold((unsigned char)query[i]))i++;
  if(!query[i])return 1;
 }return 0;
}
static unsigned int view_indices(const struct nova_files_ui*u,unsigned int*out){
 unsigned int n=0;for(unsigned int i=0;i<u->files->count&&i<NOVA_FILES_MAX_ENTRIES;i++)if((!u->folders_only||u->files->entries[i].type==NOVA_FILES_DTYPE_DIR)&&filter_matches(u->files->entries[i].name,u->name_filter))out[n++]=i;
 if(u->sort_mode){for(unsigned int i=1;i<n;i++){unsigned int value=out[i],j=i;
  while(j){const struct nova_files_entry*a=&u->files->entries[out[j-1]],*b=&u->files->entries[value];int order;
   if(u->sort_mode>=3){int av=u->metadata[out[j-1U]].valid&&a->type==8,bv=u->metadata[value].valid&&b->type==8;
    if(av!=bv)order=bv-av;else if(av&&u->metadata[out[j-1U]].bytes!=u->metadata[value].bytes){order=u->metadata[out[j-1U]].bytes>u->metadata[value].bytes?1:-1;if(u->sort_mode==4)order=-order;}else order=entry_name_order(a->name,b->name);
   }else if(u->sort_mode==2)order=a->type!=b->type?(int)a->type-(int)b->type:entry_name_order(a->name,b->name);
   else{int ad=a->type==4,bd=b->type==4;order=ad!=bd?bd-ad:-entry_name_order(a->name,b->name);}
   if(order<=0)break;out[j]=out[j-1];j--;
  }out[j]=value;
 }}return n;
}
static unsigned int view_count(const struct nova_files_ui*u){unsigned int indices[NOVA_FILES_MAX_ENTRIES];return view_indices(u,indices);}
static void clamp(struct nova_files_ui*u){
 unsigned int indices[NOVA_FILES_MAX_ENTRIES],total=view_indices(u,indices),limit=total>visible_rows(u)?total-visible_rows(u):0;
 if(u->scroll>limit)u->scroll=limit;int found=0;for(unsigned int i=0;i<total;i++)if(indices[i]==u->selected)found=1;if(!found)u->selected=~0U;
}
unsigned int nova_files_ui_match_count(const struct nova_files_ui*u){return u&&u->files?view_count(u):0;}
int nova_files_ui_find(struct nova_files_ui*u){if(!u||!u->files||u->search_focused)return 0;u->search_focused=1;return 1;}
static void refresh_metadata(struct nova_files_ui*u);
int nova_files_ui_filter_key(struct nova_files_ui*u,unsigned int key){
 if(!u||!u->files||!u->search_focused)return 0;
 char previous_filter[sizeof(u->name_filter)];for(unsigned int i=0;i<sizeof(previous_filter);i++)previous_filter[i]=u->name_filter[i];
 unsigned int n=0;while(n+1<sizeof(u->name_filter)&&u->name_filter[n])n++;
 if(key==27){u->name_filter[0]=0;u->search_focused=0;}
 else if(key==13){u->search_focused=0;return 1;}
 else if(key==8){if(!n)return 0;u->name_filter[--n]=0;}
 else if(key>=32&&key<=126&&n+1<sizeof(u->name_filter)){u->name_filter[n]=(char)key;u->name_filter[n+1]=0;}
 else return 0;
 if(u->files->scanned>NOVA_FILES_MAX_ENTRIES&&u->files->ops.read_entries){
  struct nova_files_entry selected;int had=u->selected<u->files->count;if(had)selected=u->files->entries[u->selected];
  int rc=nova_files_query(u->files,u->name_filter);
  if(rc){for(unsigned int i=0;i<sizeof(previous_filter);i++)u->name_filter[i]=previous_filter[i];u->last_error=(unsigned int)-rc;return -1;}
  u->selected=~0U;if(had)for(unsigned int i=0;i<u->files->count;i++)if(!entry_name_order(selected.name,u->files->entries[i].name)&&selected.type==u->files->entries[i].type)u->selected=i;
  u->last_error=0;u->refresh_generation++;refresh_metadata(u);
 }
 u->scroll=0;u->hovered=~0U;clamp(u);return 1;
}
static void refresh_metadata(struct nova_files_ui*u){
 for(unsigned int i=0;i<NOVA_FILES_MAX_ENTRIES;i++){u->metadata[i]=(struct nova_files_metadata){0};
  if(u->metadata_reader&&u->files&&i<u->files->count&&u->files->depth<=NOVA_FILES_MAX_DEPTH)(void)u->metadata_reader(u->files->directory_fds[u->files->depth],u->files->entries[i].name,u->files->entries[i].type,&u->metadata[i]);
 }
}
static int poll_one_metadata(struct nova_files_ui*u,unsigned i){
 struct nova_files_metadata next={0};
 (void)u->metadata_reader(u->files->directory_fds[u->files->depth],u->files->entries[i].name,u->files->entries[i].type,&next);
 const struct nova_files_metadata*old=&u->metadata[i];
 if(next.valid!=old->valid||next.bytes!=old->bytes||next.posix_valid!=old->posix_valid||next.mode!=old->mode||next.uid!=old->uid||next.gid!=old->gid){u->metadata[i]=next;return 1;}return 0;
}
int nova_files_ui_poll_metadata(struct nova_files_ui*u){
 if(!u||!u->files||!u->metadata_reader)return 0;unsigned indices[NOVA_FILES_MAX_ENTRIES],n=view_indices(u,indices),changed=0,selected_seen=0;
 unsigned end=u->scroll+visible_rows(u);if(end>n)end=n;
 for(unsigned j=u->scroll;j<end;j++){unsigned i=indices[j];if(i==u->selected)selected_seen=1;changed|=(unsigned)poll_one_metadata(u,i);}
 /* The details pane remains live when its selected row is scrolled out. */
 if(u->properties_open&&!selected_seen&&u->selected<u->files->count)changed|=(unsigned)poll_one_metadata(u,u->selected);
 if(changed){u->hovered=~0U;clamp(u);}return (int)changed;
}
static void refresh_places(struct nova_files_ui*u){
 if(!u->files||u->files->depth)return;u->place_count=0;
 for(unsigned i=0;i<u->files->count&&u->place_count<6;i++)if(u->files->entries[i].type==NOVA_FILES_DTYPE_DIR){
  unsigned j=0;while(j+1<NOVA_FILES_NAME_BYTES&&u->files->entries[i].name[j]){u->places[u->place_count][j]=u->files->entries[i].name[j];j++;}u->places[u->place_count++][j]=0;
 }
}
void nova_files_ui_set_metadata(struct nova_files_ui*u,nova_files_metadata_reader reader){if(!u)return;u->metadata_reader=reader;refresh_metadata(u);}
void nova_files_ui_init(struct nova_files_ui*u,struct nova_files_model*f){
 if(!u)return;u->metadata_reader=0;for(unsigned int i=0;i<NOVA_FILES_MAX_ENTRIES;i++){u->metadata[i]=(struct nova_files_metadata){0};}u->files=f;u->scroll=0;u->selected=~0U;u->hovered=~0U;u->hover_control=0;u->pressed_control=0;
 u->workspace_shared=0;u->preview_reader=0;u->preview_open=0;u->preview_status=0;u->preview_text[0]=0;u->properties_open=0;u->search_focused=0;u->name_filter[0]=0;u->last_error=0;u->click_count=0;u->refresh_generation=0;u->viewport_width=800;u->viewport_height=500;u->theme=2;u->folders_only=0;u->sort_mode=0;u->place_count=0;u->history_count=1;u->history_index=0;
 unsigned int hn=0;while(hn+1<NOVA_FILES_PATH_BYTES&&f&&f->path[hn]){u->history[0][hn]=f->path[hn];hn++;}u->history[0][hn]=0;
 if(f&&f->depth==0)for(unsigned int i=0;i<f->count&&u->place_count<6;i++)if(f->entries[i].type==NOVA_FILES_DTYPE_DIR){
  unsigned int j=0;while(j+1<NOVA_FILES_NAME_BYTES&&f->entries[i].name[j]){u->places[u->place_count][j]=f->entries[i].name[j];j++;}u->places[u->place_count][j]=0;u->place_count++;
 }
}
int nova_files_ui_resize(struct nova_files_ui*u,unsigned int w,unsigned int h){
 if(!u||w<640||h<400||w>1920||h>1080)return 0;u->viewport_width=w;u->viewport_height=h;clamp(u);return 1;
}
static void copy_path(char*d,const char*s){unsigned int n=0;while(n+1<NOVA_FILES_PATH_BYTES&&s[n]){d[n]=s[n];n++;}d[n]=0;}
static void history_record(struct nova_files_ui*u){
 if(u->history_count){const char*a=u->history[u->history_index],*b=u->files->path;unsigned int i=0;while(a[i]&&a[i]==b[i])i++;if(a[i]==b[i])return;}
 unsigned int next=u->history_count?u->history_index+1:0;
 if(next>=NOVA_FILES_HISTORY){for(unsigned int i=1;i<NOVA_FILES_HISTORY;i++)copy_path(u->history[i-1],u->history[i]);next=NOVA_FILES_HISTORY-1;}
 copy_path(u->history[next],u->files->path);u->history_index=next;u->history_count=next+1;
}
static int finish_navigation(struct nova_files_ui*u,int result,int record){
 if(result){u->last_error=(unsigned int)-result;return -1;}
 u->scroll=0;u->selected=~0U;u->hovered=~0U;u->name_filter[0]=0;u->search_focused=0;u->last_error=0;u->refresh_generation++;refresh_metadata(u);if(record)history_record(u);return 1;
}
static int do_page(struct nova_files_ui*u,int forward){
 if(forward?!u->files->has_next:!u->files->has_previous)return 0;
 int rc=nova_files_page(u->files,forward);if(rc){u->last_error=(unsigned int)-rc;return -1;}
 u->scroll=0;u->selected=~0U;u->hovered=~0U;u->properties_open=0;u->search_focused=0;u->last_error=0;u->refresh_generation++;refresh_metadata(u);return 1;
}
static int navigate(struct nova_files_ui*u,const char*path){return finish_navigation(u,nova_files_navigate(u->files,path),1);}
int nova_files_ui_open_desktop(struct nova_files_ui*u,const char*name){
 if(!u||!u->files||!nova_desktop_open_name(name))return -1;
 char path[NOVA_FILES_PATH_BYTES];const char*prefix="/Desktop/";unsigned i=0,j=0;
 for(;prefix[i];i++)path[i]=prefix[i];
 for(;name[j]&&i+1<sizeof path;j++)path[i++]=name[j];if(name[j])return -1;path[i]=0;
 int result=navigate(u,path);
 if(result>0){u->properties_open=0;u->preview_open=0;u->preview_status=0;u->preview_text[0]=0;}
 return result;
}

static int history_step(struct nova_files_ui*u,int forward){
 if((forward&&u->history_index+1>=u->history_count)||(!forward&&!u->history_index))return 0;
 unsigned int target=forward?u->history_index+1:u->history_index-1;
 int r=finish_navigation(u,nova_files_navigate(u->files,u->history[target]),0);if(r>0)u->history_index=target;return r;
}
static int do_refresh(struct nova_files_ui*u){
 char selected[NOVA_FILES_NAME_BYTES]={0};if(u->selected<u->files->count){unsigned i=0;while(i+1<sizeof selected&&u->files->entries[u->selected].name[i]){selected[i]=u->files->entries[u->selected].name[i];i++;}}
 int r=nova_files_list(u->files);if(r){u->last_error=(unsigned)-r;return -1;}
 u->selected=~0U;u->hovered=~0U;
 if(selected[0])for(unsigned i=0;i<u->files->count;i++)if(!entry_name_order(selected,u->files->entries[i].name)){u->selected=i;break;}
 if(u->selected==~0U)u->properties_open=0;
 u->last_error=0;u->refresh_generation++;refresh_metadata(u);refresh_places(u);clamp(u);return 1;
}
static int do_parent(struct nova_files_ui*u){
 if(!u->files->depth)return 0;char path[NOVA_FILES_PATH_BYTES];copy_path(path,u->files->path);unsigned int n=0;while(path[n])n++;while(n>1&&path[n-1]!='/')n--;path[n>1?n-1:1]=0;return navigate(u,path);
}
static int do_enter(struct nova_files_ui*u,unsigned int i){
 if(i>=u->files->count)return 0;const struct nova_files_entry*e=&u->files->entries[i];
 if(e->type!=NOVA_FILES_DTYPE_DIR){u->selected=i;
  if(e->type==8&&u->preview_reader){u->preview_text[0]=0;u->preview_status=u->preview_reader(u->files->directory_fds[u->files->depth],e->name,u->preview_text,sizeof u->preview_text);u->preview_open=1;}
  return 1;}
 return finish_navigation(u,nova_files_enter(u->files,e->name),1);
}
int nova_files_ui_scroll(struct nova_files_ui*u,int detents){
 if(!u||!u->files||!detents)return 0;unsigned int old=u->scroll,limit=view_count(u)>visible_rows(u)?view_count(u)-visible_rows(u):0;
 long long next=(long long)old-(long long)detents*3;if(next<0)next=0;if(next>limit)next=limit;u->scroll=(unsigned int)next;u->hovered=~0U;return old!=u->scroll;
}
/* Stable control IDs; visual order is Back, Forward, Parent, Reload. */
static int control_x(const struct nova_files_ui*u,unsigned int id){int side=(int)nova_files_ui_sidebar_width(u->viewport_width);return id==9?side+150:id==10?side+186:id==8?(int)u->viewport_width-264:id==7?(int)u->viewport_width-226:id==6?(int)u->viewport_width-184:id==1?side+90:id==2?side+126:id==3?(int)u->viewport_width-50:id==4?side+18:side+54;}
static int control_enabled(const struct nova_files_ui*u,unsigned int id){return id==9?u->files->has_previous:id==10?u->files->has_next:id==7?(u->properties_open||u->selected<u->files->count):id==1?u->files->depth!=0:id==4?u->history_index!=0:id==5?u->history_index+1<u->history_count:1;}
static unsigned int control_at(const struct nova_files_ui*u,int x,int y){
 if((u->files->has_previous||u->files->has_next)&&y>=(int)u->viewport_height-28&&y<(int)u->viewport_height-4)for(unsigned id=9;id<=10;id++)if(x>=control_x(u,id)&&x<control_x(u,id)+30)return id;
 if(y>=32&&y<64&&x>=control_x(u,8)&&x<control_x(u,8)+32)return 8;
 if(y>=32&&y<64&&x>=control_x(u,7)&&x<control_x(u,7)+32)return 7;
 if(y>=32&&y<64&&x>=(int)u->viewport_width-184&&x<(int)u->viewport_width-64)return 6;
 for(unsigned int id=1;id<=5;id++)if(id!=3&&y>=68&&y<104&&x>=control_x(u,id)&&x<control_x(u,id)+32)return id;
 if(y>=32&&y<64&&x>=(int)u->viewport_width-52&&x<(int)u->viewport_width-14)return 3;
 return 0;
}
struct breadcrumb { int x,w;unsigned int depth;char text[NOVA_FILES_NAME_BYTES+4]; };
static unsigned int breadcrumbs(const struct nova_files_ui*u,struct breadcrumb*out){
 int left=(int)nova_files_ui_sidebar_width(u->viewport_width)+184,right=(int)u->viewport_width-18;unsigned int count=0;
 unsigned int depths[3]={0,0,0},n=1;if(u->files->depth>1)depths[n++]=u->files->depth-1;if(u->files->depth)depths[n++]=u->files->depth;
 for(unsigned int k=0;k<n;k++){
  char label[NOVA_FILES_NAME_BYTES];unsigned int d=depths[k];
  if(!d){label[0]='F';label[1]='i';label[2]='l';label[3]='e';label[4]='s';label[5]=0;}
  else{unsigned int a=u->files->path_marks[d-1];if(u->files->path[a]=='/')a++;unsigned int z=0;while(a<u->files->path_marks[d]&&z+1<sizeof(label))label[z++]=u->files->path[a++];label[z]=0;}
  int room=(right-left)/(int)(n-k)-12;if(room<12)break;if(room>112)room=112;
  struct breadcrumb*b=&out[count++];b->x=left;b->depth=d;nova_ui_ellipsis(label,b->text,sizeof(b->text),NOVA_UI_BODY,(unsigned int)room);b->w=(int)nova_ui_measure(b->text,NOVA_UI_BODY,1)+10;left+=b->w+12;
 }
 return count;
}
int nova_files_ui_path_at(const struct nova_files_ui*u,int x,int y,char*out,unsigned cap){
 if(!u||!u->files||!u->workspace_shared||u->preview_open||u->properties_open)return 0;int side=(int)nova_files_ui_sidebar_width(u->viewport_width);
 if(x<side+8||x>=(int)u->viewport_width-22||y<128||y>=128+(int)(40U*visible_rows(u)))return 0;
 unsigned indices[NOVA_FILES_MAX_ENTRIES],n=view_indices(u,indices),pos=u->scroll+(unsigned)(y-128)/40;if(pos>=n)return 0;const struct nova_files_entry*e=&u->files->entries[indices[pos]];
 if(e->type!=NOVA_FILES_DTYPE_DIR&&e->type!=8)return 0;unsigned a=0;while(u->files->path[a])a++;unsigned b=0;while(e->name[b])b++;unsigned sep=a>1?1:0;if(a+sep+b+1>cap)return 0;
 for(unsigned i=0;i<a;i++)out[i]=u->files->path[i];if(sep)out[a++]='/';for(unsigned i=0;i<=b;i++)out[a+i]=e->name[i];return 1;
}
int nova_files_ui_pointer(struct nova_files_ui*u,int x,int y,int button,int pressed){
 if(!u||!u->files)return 0;int side=(int)nova_files_ui_sidebar_width(u->viewport_width);
 if(u->preview_open){if(button==1&&pressed&&x>=(int)u->viewport_width-66&&y>=120&&y<158){u->preview_open=0;return 1;}return 0;}
 int in_field=(u->search_focused||u->name_filter[0])&&y>=72&&y<102&&x>=side+184&&x<(int)u->viewport_width-18;
 int lost_focus=button==1&&pressed&&u->search_focused&&!in_field;
 if(lost_focus)u->search_focused=0;
 if(button==1&&!pressed&&u->pressed_control){u->pressed_control=0;return 1;}
 if(u->properties_open&&x>=(int)u->viewport_width-280&&y>=106&&y<(int)u->viewport_height-32){if(button==1&&pressed&&x>=(int)u->viewport_width-48&&y<148){u->properties_open=0;return 1;}return lost_focus;}
 if(button==1&&pressed&&(u->search_focused||u->name_filter[0])&&y>=72&&y<102&&x>=side+184&&x<(int)u->viewport_width-18){
  if(x>=(int)u->viewport_width-48&&u->name_filter[0]){u->search_focused=1;int rc=nova_files_ui_filter_key(u,27);u->search_focused=1;return rc;}
  u->search_focused=1;return 1;
 }
 if(button==0){
  unsigned int next=~0U,control=control_at(u,x,y);
  if(x>=side+8&&x<(int)u->viewport_width-22&&y>=128&&y<128+(int)(visible_rows(u)*40)){
   unsigned int indices[NOVA_FILES_MAX_ENTRIES],n=view_indices(u,indices),pos=u->scroll+(unsigned int)(y-128)/40;next=pos<n?indices[pos]:~0U;}
  if(next==u->hovered&&control==u->hover_control)return 0;u->hovered=next;u->hover_control=control;return 1;
 }
 if(button!=1)return 0;if(!pressed){if(!u->pressed_control)return 0;u->pressed_control=0;return 1;}
 if(x<0||y<0||x>=(int)u->viewport_width||y>=(int)u->viewport_height)return 0;
 if(!u->search_focused&&!u->name_filter[0]&&y>=72&&y<102){struct breadcrumb crumbs[3];unsigned int n=breadcrumbs(u,crumbs);for(unsigned int i=0;i<n;i++)if(x>=crumbs[i].x&&x<crumbs[i].x+crumbs[i].w){
  if(crumbs[i].depth==u->files->depth)return 0;char path[NOVA_FILES_PATH_BYTES];copy_path(path,u->files->path);path[crumbs[i].depth?u->files->path_marks[crumbs[i].depth]:1]=0;return navigate(u,path);
 }}
 if(y>=106&&y<128&&x>=side+50&&x<(int)u->viewport_width-20){if(u->metadata_reader&&x>=(int)u->viewport_width-118){u->sort_mode=u->sort_mode==3?4:3;clamp(u);return 1;}u->sort_mode=x>=(int)u->viewport_width-(u->metadata_reader?220:130)?2U:u->sort_mode==1?0U:1U;clamp(u);return 1;}
 u->click_count++;unsigned int control=control_at(u,x,y);u->pressed_control=control_enabled(u,control)?control:0;
 if(control==9||control==10)return do_page(u,control==10);
 if(control==8)return nova_files_ui_find(u);
 if(control==7){if(control_enabled(u,7)){u->properties_open^=1U;return 1;}return 0;}
 if(control==6){u->folders_only^=1U;u->scroll=0;clamp(u);return 1;}if(control==4||control==5)return history_step(u,control==5);if(control==1)return u->files->depth?do_parent(u):0;if(control==2)return do_refresh(u);if(control==3){u->theme=(u->theme+1U)%3U;return 1;}
 if(x>=14&&x<side-12&&y>=138&&y<182){return navigate(u,"/");}
 unsigned int shown=(u->viewport_height-250U)/34U;if(shown>u->place_count)shown=u->place_count;
 if(x>=14&&x<side-12&&y>=184&&y<184+(int)(shown*34U)){
  unsigned int place=(unsigned int)(y-184)/34U;char path[NOVA_FILES_PATH_BYTES];path[0]='/';unsigned int n=0;while(u->places[place][n]&&n+2<sizeof(path)){path[n+1]=u->places[place][n];n++;}path[n+1]=0;return navigate(u,path);
 }
 if(x>=(int)u->viewport_width-20&&x<(int)u->viewport_width-3&&y>=140&&y<164){if(u->scroll){u->scroll--;return 1;}return 0;}
 if(x>=(int)u->viewport_width-20&&x<(int)u->viewport_width-3&&y>=(int)u->viewport_height-59&&y<(int)u->viewport_height-35){if(u->scroll+visible_rows(u)<view_count(u)){u->scroll++;return 1;}return 0;}
 if(x>=(int)u->viewport_width-20&&x<(int)u->viewport_width-3&&y>=166&&y<(int)u->viewport_height-65&&view_count(u)>visible_rows(u)){
  unsigned int old=u->scroll,limit=view_count(u)-visible_rows(u);u->scroll=(unsigned int)(y-166)*limit/(u->viewport_height-232U);if(u->scroll>limit)u->scroll=limit;return old!=u->scroll;
 }
 if(x>=side+8&&x<(int)u->viewport_width-22&&y>=128&&y<128+(int)(40U*visible_rows(u))){unsigned int indices[NOVA_FILES_MAX_ENTRIES],n=view_indices(u,indices),pos=u->scroll+(unsigned int)(y-128)/40U;return pos<n?do_enter(u,indices[pos]):0;}
 return lost_focus;
}
int nova_files_ui_key(struct nova_files_ui*u,unsigned int key){
 if(!u||!u->files)return 0;unsigned int indices[NOVA_FILES_MAX_ENTRIES],n=view_indices(u,indices),pos=~0U;
 if(u->preview_open){if(key==27U||key==13U){u->preview_open=0;return 1;}return 0;}
 for(unsigned int i=0;i<n;i++)if(indices[i]==u->selected)pos=i;
 if(key==27U&&u->properties_open){u->properties_open=0;return 1;}
 if(key==27U&&(u->search_focused||u->name_filter[0])){u->search_focused=1;return nova_files_ui_filter_key(u,27);}
 if(key=='n'||key=='N')return do_page(u,1);if(key=='p'||key=='P')return do_page(u,0);
 if(key=='i'||key=='I'){if(!u->properties_open&&u->selected>=u->files->count)return 0;u->properties_open^=1U;return 1;}
 if(key=='f'||key=='F'){u->folders_only^=1;u->scroll=0;clamp(u);return 1;}
 if((key=='b'||key=='B')&&u->metadata_reader){u->sort_mode=u->sort_mode==3?4:3;clamp(u);return 1;}
 if(key=='s'||key=='S'){u->sort_mode=(u->sort_mode+1U)%3U;clamp(u);return 1;}
 if(key=='[')return history_step(u,0);if(key==']')return history_step(u,1);
 if(key=='t'||key=='T'){u->theme=(u->theme+1U)%3U;return 1;}
 if(key==8U||key==27U)return do_parent(u);if(key=='r'||key=='R')return do_refresh(u);
 if(key==33U)return nova_files_ui_scroll(u,(int)visible_rows(u)/3+1);if(key==34U)return nova_files_ui_scroll(u,-((int)visible_rows(u)/3+1));
 if(key==13U)return do_enter(u,u->selected==~0U?(u->scroll<n?indices[u->scroll]:~0U):u->selected);
 if(!n)return 0;
 if(key==36U)pos=0;else if(key==35U)pos=n-1;
 else if(key=='j'||key=='J'||key==40U){if(pos==~0U)pos=u->scroll;else if(pos+1<n)pos++;else return 0;}
 else if(key=='k'||key=='K'||key==38U){if(pos==~0U)pos=u->scroll;else if(pos)pos--;else return 0;}else return 0;
 u->selected=indices[pos];if(pos<u->scroll)u->scroll=pos;if(pos>=u->scroll+visible_rows(u))u->scroll=pos-visible_rows(u)+1U;return 1;
}
static void text(struct nova_ui_canvas c,int x,int base,unsigned int role,unsigned int col,const char*s,int width){
 if(width>0)nova_ui_text(c,x,base,role,1,col,s,x,base-24,(unsigned int)width,32);
}
static const char*entry_kind(unsigned int type){switch(type){case 4:return "Folder";case 8:return "File";case 10:return "Link";case 1:return "Pipe";case 2:case 6:return "Device";case 12:return "Socket";default:return "Unknown";}}
static void number(unsigned int value,char*out){unsigned int n=0;if(value>=10)out[n++]=(char)('0'+value/10);out[n++]=(char)('0'+value%10);out[n++]=' ';const char*s=value==1?"item":"items";while(*s)out[n++]=*s++;out[n]=0;}
static void status_text(const struct nova_files_ui*u,unsigned int count,char*out){
 number(count,out);unsigned int n=0;while(out[n])n++;
 if(u->files->has_previous||u->files->has_next){
  n=0;while(out[n]&&out[n]!=' ')n++;
  const char*label=u->name_filter[0]?(count==1?" match / ":" matches / "):" / ";for(unsigned i=0;label[i];i++)out[n++]=label[i];
  char digits[12];unsigned k=0,value=u->files->scanned;do{digits[k++]=(char)('0'+value%10);value/=10;}while(value);while(k)out[n++]=digits[--k];
  const char*tail=" seen";for(unsigned i=0;tail[i];i++)out[n++]=tail[i];out[n]=0;return;
 }
 if(u->name_filter[0]&&u->files->truncated){
  n=0;while(out[n]&&out[n]!=' ')n++;const char*label=count==1?" match / ":" matches / ";for(unsigned int i=0;label[i];i++)out[n++]=label[i];
  char digits[12];unsigned int k=0,value=u->files->scanned;do{digits[k++]=(char)('0'+value%10);value/=10;}while(value);while(k)out[n++]=digits[--k];
  const char*tail=" scanned";for(unsigned int i=0;tail[i];i++)out[n++]=tail[i];out[n]=0;return;
 }
 if(u->files->truncated||u->folders_only||u->name_filter[0]){out[n++]=' ';out[n++]='/';out[n++]=' ';unsigned int value=u->files->truncated?u->files->scanned:u->files->count;char reverse[10];unsigned int k=0;do{reverse[k++]=(char)('0'+value%10);value/=10;}while(value);while(k)out[n++]=reverse[--k];const char*tail=u->files->truncated?" scanned":" total";for(k=0;tail[k];k++)out[n++]=tail[k];out[n]=0;}
}
static void size_text(unsigned long long bytes,char*out){
 const char*units[]={" B"," KiB"," MiB"," GiB"," TiB"," PiB"," EiB"};unsigned int u=0,n=0;unsigned long long divisor=1;
 while(u<6&&bytes/divisor>=1024){divisor*=1024;u++;}
 unsigned long long whole=bytes/divisor;char reverse[24];unsigned int k=0;do{reverse[k++]=(char)('0'+whole%10);whole/=10;}while(whole);while(k)out[n++]=reverse[--k];
 if(u){out[n++]='.';out[n++]=(char)('0'+(bytes%divisor)*10/divisor);}for(k=0;units[u][k];k++)out[n++]=units[u][k];out[n]=0;
}
int nova_files_ui_render(const struct nova_files_ui*u,void*b,unsigned int w,unsigned int h,unsigned int pitch){
 if(!u||!u->files||!b||w<640||h<400||pitch<w*4U||pitch>32768||pitch%4)return 0;
 struct nova_ui_canvas c={(unsigned char*)b,w,h,pitch};struct nova_ui_palette p=nova_ui_palette_for(u->theme);
 int side=(int)nova_files_ui_sidebar_width(w);unsigned int indices[NOVA_FILES_MAX_ENTRIES],count=view_indices(u,indices);
 nova_ui_round(c,0,0,(int)w,(int)h,0,p.paper);nova_ui_round(c,0,0,(int)w,106,0,p.toolbar);
 nova_ui_round(c,0,28,side,(int)h-28,0,p.sidebar);nova_ui_round(c,side,28,1,(int)h-28,0,p.line);
 text(c,24,66,NOVA_UI_HEADING,p.ink,"Private files",side-40);
 text(c,24,88,NOVA_UI_CAPTION,p.muted,u->workspace_shared?"Shared with Terminal":"Private workspace",side-38);
 text(c,24,125,NOVA_UI_CAPTION,p.muted,"Locations",side-38);
 if(!u->files->depth)nova_ui_round(c,14,138,side-28,40,8,p.selection);
 nova_ui_folder(c,27,150,18,p.accent);text(c,55,163,NOVA_UI_HEADING,p.ink,"My files",side-68);
 unsigned int shown=(h-250U)/34U;if(shown>u->place_count)shown=u->place_count;
 for(unsigned int i=0;i<shown;i++){
  int yy=184+(int)i*34;char fitted[NOVA_FILES_NAME_BYTES+4];nova_ui_ellipsis(u->places[i],fitted,sizeof(fitted),NOVA_UI_BODY,(unsigned int)(side-72));
  if(u->files->depth){unsigned int a=1,j=0;while(u->places[i][j]&&u->files->path[a]==u->places[i][j]){a++;j++;}if(!u->places[i][j]&&(!u->files->path[a]||u->files->path[a]=='/'))nova_ui_round(c,14,yy,side-28,32,7,p.selection);}
  nova_ui_folder(c,28,yy+9,16,p.accent);text(c,55,yy+22,NOVA_UI_BODY,p.ink,fitted,side-67);
 }
 text(c,24,(int)h-43,NOVA_UI_CAPTION,p.muted,"Read-only",side-38);
 text(c,24,(int)h-24,NOVA_UI_CAPTION,p.muted,"Scoped access",side-38);
 const char*heading="My files";if(u->files->depth){heading=u->files->path;for(const char*q=u->files->path;*q;q++)if(*q=='/'&&q[1])heading=q+1;}
 char fitted[NOVA_FILES_NAME_BYTES+4];nova_ui_ellipsis(heading,fitted,sizeof(fitted),NOVA_UI_TITLE,w-(unsigned int)side-264U);
 text(c,side+18,57,NOVA_UI_TITLE,p.ink,fitted,(int)w-side-250);
 for(unsigned int control=1;control<=8;control++){
  int xx=control_x(u,control),yy=control==3||control>=6?34:72;unsigned int state=!control_enabled(u,control)?NOVA_UI_DISABLED:0;
  if(u->hover_control==control)state|=NOVA_UI_HOVER;if(u->pressed_control==control)state|=NOVA_UI_PRESSED;
  (void)nova_ui_button(c,xx,yy,control==6?120:32,30,p,state|(control==6&&u->folders_only?NOVA_UI_SELECTED:0));
 }
 int find_x=control_x(u,8)+9;
 nova_ui_round(c,find_x,42,12,12,6,p.ink);nova_ui_round(c,find_x+2,44,8,8,4,p.toolbar);nova_ui_line(c,find_x+10,52,find_x+15,57,p.ink);
 text(c,control_x(u,7)+12,54,NOVA_UI_HEADING,control_enabled(u,7)?p.ink:p.muted,"i",20);
 text(c,(int)w-174,54,NOVA_UI_CAPTION,p.ink,u->folders_only?"Folders only":"All items",108);
 for(unsigned int id=4;id<=5;id++){int bx=control_x(u,id)+16;unsigned int ink=control_enabled(u,id)?p.ink:p.muted;int direction=id==4?-1:1;
  nova_ui_line(c,bx-direction*3,81,bx+direction*3,87,ink);nova_ui_line(c,bx+direction*3,87,bx-direction*3,93,ink);
 }
 unsigned int up=u->files->depth?p.ink:p.muted;int x=control_x(u,1)+16;
 nova_ui_line(c,x-5,87,x+1,81,up);nova_ui_line(c,x+1,81,x+7,87,up);nova_ui_line(c,x+1,81,x+1,94,up);
 x=control_x(u,2)+16;/* Recognisable open circular reload arrow, not the former square arrow. */
 int points[][2]={{3,-7},{7,-3},{7,3},{3,7},{-3,7},{-7,3},{-7,-3},{-4,-6}};
 for(unsigned int i=0;i<7;i++)nova_ui_line(c,x+points[i][0],87+points[i][1],x+points[i+1][0],87+points[i+1][1],p.ink);
 nova_ui_line(c,x-4,81,x-4,85,p.ink);nova_ui_line(c,x-4,81,x-8,81,p.ink);
 nova_ui_round(c,(int)w-39,41,14,14,7,p.ink);nova_ui_round(c,(int)w-35,40,11,14,5,p.toolbar);
 if(u->search_focused||u->name_filter[0]){
  int left=side+184,wide=(int)w-left-18;unsigned int state=u->search_focused?NOVA_UI_FOCUSED:0;
  (void)nova_ui_button(c,left,72,wide,30,p,state);
  const char*shown_filter=u->name_filter;int room=wide-48;
  while(*shown_filter&&nova_ui_measure(shown_filter,NOVA_UI_BODY,1)>(unsigned int)room)shown_filter++;
  text(c,left+10,93,NOVA_UI_BODY,u->name_filter[0]?p.ink:p.muted,u->name_filter[0]?shown_filter:"Filter this folder (Ctrl+F)",room);
  if(u->search_focused){int caret=left+10+(int)nova_ui_measure(shown_filter,NOVA_UI_BODY,1);nova_ui_line(c,caret,80,caret,96,p.accent);}
  if(u->name_filter[0]){int cx=(int)w-35;nova_ui_line(c,cx-3,83,cx+3,89,p.muted);nova_ui_line(c,cx+3,83,cx-3,89,p.muted);}
 }else{
 struct breadcrumb crumbs[3];unsigned int nc=breadcrumbs(u,crumbs);
 for(unsigned int i=0;i<nc;i++){
  struct breadcrumb*b=&crumbs[i];if(b->depth==u->files->depth)nova_ui_round(c,b->x-2,74,b->w,26,5,p.hover);
  text(c,b->x+3,93,NOVA_UI_BODY,b->depth==u->files->depth?p.ink:p.muted,b->text,b->w-3);
  if(i+1<nc){int xx=b->x+b->w+5;nova_ui_line(c,xx,83,xx+4,87,p.muted);nova_ui_line(c,xx+4,87,xx,91,p.muted);}
 }

 }
 nova_ui_round(c,side+12,106,(int)w-side-24,1,0,p.line);
 text(c,side+64,122,NOVA_UI_CAPTION,u->sort_mode<2?p.accent:p.muted,(u->files->has_previous||u->files->has_next)?"Name / page":"Name",(int)w-side-190);text(c,(int)w-(u->metadata_reader?204:113),122,NOVA_UI_CAPTION,u->sort_mode==2?p.accent:p.muted,"Type",88);
 if(u->metadata_reader)text(c,(int)w-104,122,NOVA_UI_CAPTION,u->sort_mode>=3?p.accent:p.muted,"Size",80);
 int ax=u->sort_mode>=3?(int)w-64:u->sort_mode==2?(int)w-(u->metadata_reader?164:76):(u->files->has_previous||u->files->has_next)?side+160:side+109,ay=116;int down=(u->sort_mode==1||u->sort_mode==4)?1:-1;
 nova_ui_line(c,ax-3,ay-down*2,ax,ay+down*2,p.accent);nova_ui_line(c,ax,ay+down*2,ax+3,ay-down*2,p.accent);
 for(unsigned int row=0;row<visible_rows(u);row++){
  unsigned int position=u->scroll+row;if(position>=count)break;unsigned int i=indices[position];const struct nova_files_entry*entry=&u->files->entries[i];int yy=128+(int)row*40;
  if(u->selected==i||u->hovered==i)nova_ui_round(c,side+10,yy+1,(int)w-side-34,38,7,u->selected==i?p.selection:p.hover);
  if(u->selected==i)nova_ui_round(c,side+10,yy+10,3,20,1,p.accent);
  int folder=entry->type==NOVA_FILES_DTYPE_DIR;
  if(folder)nova_ui_folder(c,side+26,yy+10,24,u->theme==1?0x008ab7d3:0x005788b0);
  else nova_ui_document(c,side+28,yy+9,23,u->theme==1?0x009dabc0:0x0079889e);
  nova_ui_ellipsis(entry->name,fitted,sizeof(fitted),NOVA_UI_BODY,w-(unsigned int)side-(u->metadata_reader?292U:204U));
  text(c,side+64,yy+25,NOVA_UI_BODY,p.ink,fitted,(int)w-side-(u->metadata_reader?292:204));
  text(c,(int)w-(u->metadata_reader?204:113),yy+25,NOVA_UI_CAPTION,p.muted,entry_kind(entry->type),88);
  if(u->metadata_reader){char size[40];if(u->metadata[i].valid&&entry->type==8)size_text(u->metadata[i].bytes,size);else{size[0]='-';size[1]=0;}text(c,(int)w-104,yy+25,NOVA_UI_CAPTION,p.muted,size,88);}
 }
 if(!count){int mid=(side+(int)w)/2;nova_ui_folder(c,mid-24,198,48,p.accent);
  const char*empty=u->name_filter[0]?"No matching names":u->folders_only&&u->files->count?"No folders in this view":"This folder is empty";int ew=(int)nova_ui_measure(empty,NOVA_UI_HEADING,1);text(c,mid-ew/2,278,NOVA_UI_HEADING,p.ink,empty,(int)w-side-36);
  const char*hint=u->name_filter[0]?"Clear filter or use page arrows (N/P)":u->folders_only&&u->files->count?"Show all items to see files":"Nothing to display here";int hw=(int)nova_ui_measure(hint,NOVA_UI_BODY,1);text(c,mid-hw/2,302,NOVA_UI_BODY,p.muted,hint,(int)w-side-36);}
 if(count>visible_rows(u)){
  int sx=(int)w-12,top=148,bottom=(int)h-47;unsigned int ink=u->scroll?p.ink:p.muted;
  nova_ui_line(c,sx-4,top+3,sx,top-1,ink);nova_ui_line(c,sx,top-1,sx+4,top+3,ink);
  nova_ui_line(c,sx-4,bottom-3,sx,bottom+1,p.muted);nova_ui_line(c,sx,bottom+1,sx+4,bottom-3,p.muted);
  int track=bottom-top-36,thumb=track*(int)visible_rows(u)/(int)count;if(thumb<18)thumb=18;
  int offset=(track-thumb)*(int)u->scroll/(int)(count-visible_rows(u));nova_ui_round(c,sx-2,top+18+offset,4,thumb,2,p.muted);
 }
 if(u->properties_open){int left=(int)w-280;unsigned int valid=u->selected<u->files->count;const struct nova_files_entry*entry=valid?&u->files->entries[u->selected]:0;
  nova_ui_round(c,left,107,280,(int)h-139,0,p.paper);nova_ui_line(c,left,107,left,(int)h-32,p.line);text(c,left+20,140,NOVA_UI_HEADING,p.ink,"File details",216);text(c,(int)w-38,140,NOVA_UI_BODY,p.muted,"x",20);
  if(entry){char name[NOVA_FILES_NAME_BYTES+4],bytes[40];const struct nova_files_metadata*m=&u->metadata[u->selected];
   nova_ui_ellipsis(entry->name,name,sizeof(name),NOVA_UI_BODY,240);text(c,left+20,178,NOVA_UI_BODY,p.ink,name,240);
   text(c,left+20,210,NOVA_UI_CAPTION,p.muted,"Type",108);text(c,left+142,210,NOVA_UI_BODY,p.ink,entry_kind(entry->type),118);
   text(c,left+20,240,NOVA_UI_CAPTION,p.muted,"Size",108);if(m->valid&&entry->type==8)size_text(m->bytes,bytes);else{const char*t="Unavailable";unsigned j=0;while(t[j]){bytes[j]=t[j];j++;}bytes[j]=0;}text(c,left+142,240,NOVA_UI_BODY,p.ink,bytes,118);
   text(c,left+20,270,NOVA_UI_CAPTION,p.muted,"Mode",108);
   if(m->valid&&m->posix_valid){char octal[5],symbolic[10],owner[11],group[11];nova_ui_mode_octal(m->mode,octal);nova_ui_permissions(m->mode,symbolic);nova_ui_unsigned(m->uid,owner);nova_ui_unsigned(m->gid,group);
    text(c,left+142,270,NOVA_UI_BODY,p.ink,octal,118);text(c,left+142,294,NOVA_UI_CAPTION,p.muted,symbolic,118);
    text(c,left+20,324,NOVA_UI_CAPTION,p.muted,"Owner UID",108);text(c,left+142,324,NOVA_UI_BODY,p.ink,owner,118);
    text(c,left+20,354,NOVA_UI_CAPTION,p.muted,"Group GID",108);text(c,left+142,354,NOVA_UI_BODY,p.ink,group,118);
   }else{text(c,left+142,270,NOVA_UI_CAPTION,p.muted,"Unavailable",118);}
   if(h>=480){text(c,left+20,389,NOVA_UI_CAPTION,p.muted,"Location",240);nova_ui_ellipsis(u->files->path,name,sizeof(name),NOVA_UI_BODY,240);text(c,left+20,414,NOVA_UI_BODY,p.ink,name,240);text(c,left+20,(int)h-42,NOVA_UI_CAPTION,p.muted,"Mode bits / read-only view",240);}
  }else{text(c,left+20,190,NOVA_UI_BODY,p.muted,"Select an item",240);}
 }
 if(u->preview_open){
  int left=side+16,width=(int)w-side-32;
  nova_ui_round(c,left,112,width,(int)h-152,10,p.paper);
  text(c,left+18,142,NOVA_UI_HEADING,p.ink,"Text preview",width-80);text(c,(int)w-56,142,NOVA_UI_BODY,p.muted,"x",20);
  text(c,left+18,166,NOVA_UI_CAPTION,p.muted,"Read-only / up to 4 KiB / Esc to close",width-36);
  if(u->preview_status<0)text(c,left+18,208,NOVA_UI_BODY,p.accent,u->preview_status==-13?"Permission denied":u->preview_status==-27?"File is too large for preview":"Preview unavailable",width-36);
  else if(!u->preview_status)text(c,left+18,208,NOVA_UI_BODY,p.muted,"Empty file",width-36);
  else{unsigned at=0;int yy=202;while(u->preview_text[at]&&yy<(int)h-58){char line[160];unsigned len=0;while(u->preview_text[at]&&u->preview_text[at]!='\n'){char ch=u->preview_text[at++];if(len+1<sizeof line)line[len++]=ch=='\t'?' ':ch;}if(u->preview_text[at]=='\n')at++;line[len]=0;text(c,left+18,yy,NOVA_UI_BODY,p.ink,line,width-36);yy+=24;}}
 }
 nova_ui_round(c,side,(int)h-32,(int)w-side,32,0,p.toolbar);nova_ui_round(c,side,(int)h-32,(int)w-side,1,0,p.line);
 char count_text[64];status_text(u,count,count_text);int paged=u->files->has_previous||u->files->has_next;
 text(c,side+18,(int)h-11,NOVA_UI_CAPTION,p.muted,count_text,paged?128:(int)w-side-150);
 if(paged&&!u->last_error){for(unsigned id=9;id<=10;id++){
  int xx=control_x(u,id),yy=(int)h-28;unsigned state=(!control_enabled(u,id)||u->preview_open)?NOVA_UI_DISABLED:0;
  if(u->hover_control==id)state|=NOVA_UI_HOVER;if(u->pressed_control==id)state|=NOVA_UI_PRESSED;
  unsigned color=nova_ui_button(c,xx,yy,30,24,p,state);int center=xx+15,dir=id==9?-1:1;
  nova_ui_line(c,center-dir*3,yy+7,center+dir*2,yy+12,color);nova_ui_line(c,center+dir*2,yy+12,center-dir*3,yy+17,color);
 }}
 if(!u->last_error&&u->selected<u->files->count){char status[64]="1 selected",size[40];unsigned int i=u->selected;if(u->metadata_reader&&u->metadata[i].valid&&u->files->entries[i].type==8){size_text(u->metadata[i].bytes,size);unsigned int n=10;status[n++]=' ';status[n++]='/';status[n++]=' ';for(unsigned int k=0;size[k]&&n+1<sizeof(status);k++)status[n++]=size[k];status[n]=0;}text(c,(int)w-220,(int)h-11,NOVA_UI_CAPTION,p.accent,status,204);}
 if(u->last_error)text(c,side+150,(int)h-11,NOVA_UI_CAPTION,p.accent,u->last_error==(unsigned int)-NOVA_FILES_ESCAPE?"Unsafe path rejected":"Cannot open folder",(int)w-side-168);
 return 1;
}
