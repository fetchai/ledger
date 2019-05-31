DOCKER_IMAGE_NAME = 'gcr.io/organic-storm-201412/fetch-ledger-develop:v0.2.0'
HIGH_LOAD_NODE_LABEL = 'ledger'

enum Platform
{
  CLANG6('Clang 6', 'clang-6.0', 'clang++-6.0'),
  GCC7  ('GCC 7',   'gcc',       'g++')

  public Platform(label, cc, cxx)
  {
    this.label = label
    this.env_cc = cc
    this.env_cxx = cxx
  }

  public final String label
  public final String env_cc
  public final String env_cxx
}

enum Configuration
{
  DEBUG  ('Debug'),
  RELEASE('Release')

  public Configuration(label)
  {
    this.label = label
  }

  public final String label
}

// Only execute long-running tests on master and merge branches
def should_run_slow_tests()
{
  return BRANCH_NAME == 'master' || BRANCH_NAME ==~ /^PR-\d+-merge$/
}

def static_analysis()
{
  return {
    stage('Static Analysis') {
      node {
        stage('SCM Static Analysis') {
          checkout scm
        }
                        
        stage('Run Static Analysis') {
          docker.image(DOCKER_IMAGE_NAME).inside {
            sh '''\
              mkdir -p build-analysis
              cd build-analysis
              cmake ..
            '''
            sh './scripts/run_static_analysis.py build-analysis'
          }
        }
      }
    }
  }
}

def SLOW_stage(name, steps)
{
  if (should_run_slow_tests())
  {
    stage(name) { steps() }
  }
}

def create_build(Platform platform, Configuration config)
{
  def suffix = "${platform.label} ${config.label}"

  def environment = {
    return [
      "CC=${platform.env_cc}",
      "CXX=${platform.env_cxx}"
    ]
  }

  return {
    stage(suffix) {
      node(HIGH_LOAD_NODE_LABEL) {
        timeout(120) {
          stage("SCM ${suffix}") {
            checkout scm
          }

          withEnv(environment()) {
            docker.image(DOCKER_IMAGE_NAME).inside {
              stage("Build ${suffix}") {
                sh "./scripts/ci-tool.py -B ${config.label}"
              }

              stage("Unit Tests ${suffix}") {
                sh "./scripts/ci-tool.py -T ${config.label}"
              }

              SLOW_stage("Slow Tests ${suffix}") {
                sh "./scripts/ci-tool.py -S ${config.label}"
              }

              SLOW_stage("Integration Tests ${suffix}") {
                sh "./scripts/ci-tool.py -I ${config.label}"
              }

              SLOW_stage("End-to-End Tests ${suffix}") {
                sh './scripts/ci/install-test-dependencies.sh'
                sh "./scripts/ci-tool.py -E ${config.label}"
              }
            }
          }
        }
      }
    }
  }
}

def run_builds_in_parallel()
{
  def stages = [:]

  for (config in Configuration.values()) {
    for (platform in Platform.values()) {
      stages["${platform.label} ${config.label}"] = create_build(platform, config)
    }
  }

  stages['Static Analysis'] = static_analysis()

  stage('Build and Test') {
    // Execute stages
    parallel stages
  }
}

def run_basic_checks()
{
  stage('Basic Checks') {
    node {
      stage('SCM Basic Checks') {
        checkout scm
      }

      docker.image(DOCKER_IMAGE_NAME).inside {
        stage('Style Check') {
          sh './scripts/apply_style.py -d'
        }
      }
    }
  }
}

def main()
{
  timeout(180) {
    run_basic_checks()
    run_builds_in_parallel()
  }
}

// Entry point
main()
