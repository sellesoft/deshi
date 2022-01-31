#ifndef DESHI_PROFILING_H
#define DESHI_PROFILING_H


/*

Wrappers around Tracy's defines

to use these make sure tracy is included by you compiler and TRACY_ENABLE define is set

if you want the program to wait until tracy is connected define DESHI_WAIT_FOR_TRACY_CONNECTION

*/


#ifdef TRACY_ENABLE
#define DPZoneNamed(varname, active)                        ZoneNamed(varname, active)                
#define DPZoneNamedN(varname, name, active)                 ZoneNamedN(varname, name, active)         
#define DPZoneNamedC(varname, color, active)                ZoneNamedC(varname, color, active)        
#define DPZoneNamedNC(varname, name, color, active)         ZoneNamedNC(varname, name, color, active) 
#define DPZoneTransient(varname, active)                    ZoneTransient(varname, active)            
#define DPZoneTransientN(varname, name, active)             ZoneTransientN(varname, name, active)     

#define DPZoneScoped                                        ZoneScoped 
#define DPZoneScopedN(name)                                 ZoneScopedN(name)
#define DPZoneScopedC(color)                                ZoneScopedC(color)
#define DPZoneScopedNC(name, color)                         ZoneScopedNC(name, color)

#define DPZoneText(txt, size)                               ZoneText(txt, size)
#define DPZoneTextV(varname, txt, size)                     ZoneTextV(varname, txt, size)
#define DPZoneName(txt, size)                               ZoneName(txt, size)
#define DPZoneNameV(varname, txt, size)                     ZoneNameV(varname, txt, size)
#define DPZoneColor(color)                                  ZoneColor(color)
#define DPZoneColorV(varname, color)                        ZoneColorV(varname, color)
#define DPZoneValue(value)                                  ZoneValue(value)
#define DPZoneValueV(varname, value)                        ZoneValueV(varname, value)
#define DPZoneIsActive                                      ZoneIsActive
#define DPZoneIsActiveV(varname)                            ZoneIsActiveV(varname)

#define DPFrameMark                                         FrameMark 
#define DPFrameMarkNamed(name)                              FrameMarkNamed(name)
#define DPFrameMarkStart(name)                              FrameMarkStart(name)
#define DPFrameMarkEnd(name)                                FrameMarkEnd(name)

#define DPFrameImage(image, width, height, offset, flip)    FrameImage(image, width, height, offset, flip)

#define DPTracyLockable(type, varname)                      TracyLockable(type, varname)
#define DPTracyLockableN(type, varname, desc)               TracyLockableN(type, varname, desc)
#define DPTracySharedLockable(type, varname)                TracySharedLockable(type, varname)
#define DPTracySharedLockableN(type, varname, desc)         TracySharedLockableN(type, varname, desc)
#define DPLockableBase(type)                                LockableBase(type)
#define DPSharedLockableBase(type)                          SharedLockableBase(type)
#define DPLockMark(varname)                                 LockMark(varname)
#define DPLockableName(varname, txt, size)                  LockableName(varname, txt, size)

#define DPTracyPlot(name, val)                              TracyPlot(name, val)
#define DPTracyPlotConfig(name, type)                       TracyPlotConfig(name, type)

#define DPTracyAppInfo(txt, size)                           TracyAppInfo(txt, size)
#define DPTracyMessage(txt, size)                          TracyMessage(txt, size)                           
#define DPTracyMessageL(txt)                               TracyMessageL(txt)                                
#define DPTracyMessageC(txt, size, color)                  TracyMessageC(txt, size, color)                   
#define DPTracyMessageLC(txt, color)                       TracyMessageLC(txt, color)                        

#define DPTracyAlloc(ptr, size)                            TracyAlloc(ptr, size)                             
#define DPTracyFree(ptr)                                   TracyFree(ptr)                                    
#define DPTracySecureAlloc(ptr, size)                      TracySecureAlloc(ptr, size)                       
#define DPTracySecureFree(ptr)                             TracySecureFree(ptr)                              

#define DPTracyAllocN(ptr, size, name)                     TracyAllocN(ptr, size, name)                      
#define DPTracyFreeN(ptr, name)                            TracyFreeN(ptr, name)                             
#define DPTracySecureAllocN(ptr, size, name)               TracySecureAllocN(ptr, size, name)                
#define DPTracySecureFreeN(ptr, name)                      TracySecureFreeN(ptr, name)                       

#define DPZoneNamedS(varname, depth, active)               ZoneNamedS(varname, depth, active)                
#define DPZoneNamedNS(varname, name, depth, active)        ZoneNamedNS(varname, name, depth, active)         
#define DPZoneNamedCS(varname, color, depth, active)       ZoneNamedCS(varname, color, depth, active)        
#define DPZoneNamedNCS(varname, name, color, depth, active)ZoneNamedNCS(varname, name, color, depth, active) 

#define DPZoneTransientS(varname, depth, active)           ZoneTransientS(varname, depth, active)            
#define DPZoneTransientNS(varname, name, depth, active)    ZoneTransientNS(varname, name, depth, active)     

#define DPZoneScopedS(depth)                               ZoneScopedS(depth)                                
#define DPZoneScopedNS(name, depth)                        ZoneScopedNS(name, depth)                         
#define DPZoneScopedCS(color, depth)                       ZoneScopedCS(color, depth)                        
#define DPZoneScopedNCS(name, color, depth)                ZoneScopedNCS(name, color, depth)                 

#define DPTracyAllocS(ptr, size, depth)                    TracyAllocS(ptr, size, depth)                     
#define DPTracyFreeS(ptr, depth)                           TracyFreeS(ptr, depth)                            
#define DPTracySecureAllocS(ptr, size, depth)              TracySecureAllocS(ptr, size, depth)               
#define DPTracySecureFreeS(ptr, depth)                     TracySecureFreeS(ptr, depth)                      

#define DPTracyAllocNS(ptr, size, depth, name)             TracyAllocNS(ptr, size, depth, name)              
#define DPTracyFreeNS(ptr, depth, name)                    TracyFreeNS(ptr, depth, name)                     
#define DPTracySecureAllocNS(ptr, size, depth, name)       TracySecureAllocNS(ptr, size, depth, name)        
#define DPTracySecureFreeNS(ptr, depth, name)              TracySecureFreeNS(ptr, depth, name)               

#define DPTracyMessageS(txt, size, depth)                  TracyMessageS(txt, size, depth)                   
#define DPTracyMessageLS(txt, depth)                       TracyMessageLS(txt, depth)                        
#define DPTracyMessageCS(txt, size, color, depth)          TracyMessageCS(txt, size, color, depth)           
#define DPTracyMessageLCS(txt, color, depth)               TracyMessageLCS(txt, color, depth)                
#else //DESHI_RENDERER_PROFILING
#define DPZoneNamed(varname, active)                        
#define DPZoneNamedN(varname, name, active)                 
#define DPZoneNamedC(varname, color, active)                
#define DPZoneNamedNC(varname, name, color, active)         
#define DPZoneTransient(varname, active)                    
#define DPZoneTransientN(varname, name, active)             
#define DPZoneScoped                                        
#define DPZoneScopedN(name)                                 
#define DPZoneScopedC(color)                                
#define DPZoneScopedNC(name, color)                         
#define DPZoneText(txt, size)                               
#define DPZoneTextV(varname, txt, size)                     
#define DPZoneName(txt, size)                               
#define DPZoneNameV(varname, txt, size)                     
#define DPZoneColor(color)                                  
#define DPZoneColorV(varname, color)                        
#define DPZoneValue(value)                                  
#define DPZoneValueV(varname, value)                        
#define DPZoneIsActive                                      
#define DPZoneIsActiveV(varname)                            
#define DPFrameMark                                         
#define DPFrameMarkNamed(name)                              
#define DPFrameMarkStart(name)                              
#define DPFrameMarkEnd(name)                                
#define DPFrameImage(image, width, height, offset, flip)    
#define DPTracyLockable(type, varname)                      
#define DPTracyLockableN(type, varname, desc)               
#define DPTracySharedLockable(type, varname)                
#define DPTracySharedLockableN(type, varname, desc)         
#define DPLockableBase(type)                                
#define DPSharedLockableBase(type)                          
#define DPLockMark(varname)                                 
#define DPLockableName(varname, txt, size)                  
#define DPTracyPlot(name, val)                              
#define DPTracyPlotConfig(name, type)                       
#define DPTracyAppInfo(txt, size)    
#define DPTracyMessage(txt, size)
#define DPTracyMessageL(txt)                               
#define DPTracyMessageC(txt, size, color)
#define DPTracyMessageLC(txt, color)

#define DPTracyAlloc(ptr, size)
#define DPTracyFree(ptr)      
#define DPTracySecureAlloc(ptr, size)
#define DPTracySecureFree(ptr)

#define DPTracyAllocN(ptr, size, name)
#define DPTracyFreeN(ptr, name)
#define DPTracySecureAllocN(ptr, size, name)
#define DPTracySecureFreeN(ptr, name)

#define DPZoneNamedS(varname, depth, active)
#define DPZoneNamedNS(varname, name, depth, active)
#define DPZoneNamedCS(varname, color, depth, active)
#define DPZoneNamedNCS(varname, name, color, depth, active)

#define DPZoneTransientS(varname, depth, active)
#define DPZoneTransientNS(varname, name, depth, active)

#define DPZoneScopedS(depth)                               
#define DPZoneScopedNS(name, depth)
#define DPZoneScopedCS(color, depth)
#define DPZoneScopedNCS(name, color, depth)

#define DPTracyAllocS(ptr, size, depth)
#define DPTracyFreeS(ptr, depth)
#define DPTracySecureAllocS(ptr, size, depth)
#define DPTracySecureFreeS(ptr, depth)

#define DPTracyAllocNS(ptr, size, depth, name)
#define DPTracyFreeNS(ptr, depth, name)
#define DPTracySecureAllocNS(ptr, size, depth, name)
#define DPTracySecureFreeNS(ptr, depth, name)

#define DPTracyMessageS(txt, size, depth)
#define DPTracyMessageLS(txt, depth)
#define DPTracyMessageCS(txt, size, color, depth)
#define DPTracyMessageLCS(txt, color, depth)
#endif
#endif //DESHI_PROFILING_H