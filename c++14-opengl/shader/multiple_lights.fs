#version 330 core
out vec4 FragColor;

struct Material {
  sampler2D diffuse;
  sampler2D specular;
  float shininess;
};

struct DirLight {
  vec3 direction; // view coordinate

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct PointLight {
  vec3 position;

  float constant;
  float linear;
  float quadratic;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

struct SpotLight {
  vec3 position;
  vec3 direction;
  float cutoff;
  float outer_cutoff;

  float constant;
  float linear;
  float quadratic;

  vec3 ambient;
  vec3 diffuse;
  vec3 specular;
};

#define NR_POINT_LIGHTS 4

in VS_OUT {
  vec3 FragPos;
  vec3 Normal;
  vec2 TexCoords;
} fs_in;

uniform DirLight dir_light;
uniform PointLight point_lights[NR_POINT_LIGHTS];
uniform SpotLight spot_light;
uniform Material material;

vec3 calc_dir_light(DirLight light, vec3 view_dir);
vec3 calc_point_light(PointLight light, vec3 view_dir);
vec3 calc_spot_light(SpotLight light, vec3 view_dir);

void main()
{
  // properties
  vec3 view_dir = normalize(-fs_in.FragPos);

  // directional lighting
  vec3 result = calc_dir_light(dir_light, view_dir);
  // point lights
  for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
    result += calc_point_light(point_lights[i], view_dir);
  }
  // spot light
  result += calc_spot_light(spot_light, view_dir);

  FragColor = vec4(result, 1.f);
}

// calculates the color when using a directional light
vec3 calc_dir_light(DirLight light, vec3 view_dir)
{
  vec3 light_dir = normalize(-light.direction);
  // diffuse shading
  float diff = max(dot(fs_in.Normal, light_dir), 0.f);
  // specular shading
  vec3 reflect_dir = reflect(-light_dir, fs_in.Normal);
  float spec = 0.f;
  if (diff > 0.f) {
    spec = pow(max(dot(view_dir, reflect_dir), 0.f), material.shininess);
  }
  // combine results
  vec3 ambient = light.ambient * texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 diffuse = light.diffuse * diff * texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 specular = light.specular * spec * texture(material.specular, fs_in.TexCoords).rgb;

  return ambient + diffuse + specular;
}

// calculates the color when using a point light
vec3 calc_point_light(PointLight light, vec3 view_dir)
{
  vec3 light_dir = normalize(light.position - fs_in.FragPos);
  
  // diffuse shading
  float diff = max(dot(fs_in.Normal, light_dir), 0.f);
  // specular shading
  vec3 reflect_dir = reflect(-light_dir, fs_in.Normal);
  float spec = 0.f;
  if (diff > 0.f) {
    spec = pow(max(dot(view_dir, reflect_dir), 0.f), material.shininess);
  }
  // attenuation
  float distance = length(light.position - fs_in.FragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

  // combine result
  vec3 ambient = light.ambient * texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 diffuse = light.diffuse * diff * texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 specular = light.specular * spec * texture(material.specular, fs_in.TexCoords).rgb;

  return (ambient + diffuse + specular) * attenuation;
}

// calculates the color when using a spot light
vec3 calc_spot_light(SpotLight light, vec3 view_dir)
{
  vec3 light_dir = normalize(light.position - fs_in.FragPos);
  // diffuse shading
  float diff = max(dot(fs_in.Normal, light_dir), 0.f);
  // specular shading
  vec3 reflect_dir = reflect(-light_dir, fs_in.Normal);
  float spec = 0.f;
  if (diff > 0.f) {
    spec = pow(max(dot(view_dir, reflect_dir), 0.f), material.shininess);
  }
  // attenuation
  float distance = length(light.position - fs_in.FragPos);
  float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    
  // spotlight intensity
  float theta = dot(light_dir, normalize(-light.direction));
  float epsilon = light.cutoff - light.outer_cutoff;
  float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.f, 1.f);
  // combine results
  vec3 ambient = light.ambient * texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 diffuse = light.diffuse * diff * texture(material.diffuse, fs_in.TexCoords).rgb;
  vec3 specular = light.specular * spec * texture(material.specular, fs_in.TexCoords).rgb;
  
  return ambient * attenuation + (diffuse + specular) * attenuation * intensity;
}
